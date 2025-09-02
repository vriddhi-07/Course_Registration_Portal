#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

typedef struct
{
    char username[50];
    char password[50];
    char role[10]; // "admin", "student", "faculty"
    int blocked;    // 0 = active, 1 = blocked
} User;

typedef struct {
    char code[20];
    char name[50];
    int capacity;
    char faculty[50];
    int enrolled;
} Course;

typedef struct {
    char studentId[20];
    char courseIds[10][20];
    int n;
} StudentEnrollment;

#define MAX_USERS 100
#define MAX_COURSES 100
User users[MAX_USERS];
Course courses[MAX_COURSES];
StudentEnrollment students[MAX_USERS];
int userCount = 0;
int courseCount = 0;
int studentCount = 0;

void loadData();
void saveUserToFile(User *user);
void *handleClient(void *clientSocket);
void login(int clientSocket);
void adminMenu(int clientSocket);
void addStudent(int clientSocket);
void displayStudents(int clientSocket);
void addFaculty(int clientSocket);
void displayFaculty(int clientSocket);
void updateUsers();
void blockStudent(int clientSocket);
void activateStudent(int clientSocket);
void modifyStudent(int clientSocket);
void modifyFaculty(int clientSocket);
void addCourse(char *instructor, int clientSocket);
void viewCourses(char *instructor, int clientSocket);
void removeCourse(char *instructor, int clientSocket);
void updateCourse(char *instructor, int clientSocket);
void viewAllCourses(int clientSocket);
void enroll(char *studentId, int clientSocket);
void drop(char *studentId, int clientSocket);
void viewEnrolled(char *studentId, int clientSocket);
void viewEnrollments(int clientSocket);

int read_lock(int fd)
{
    struct flock lock;
    lock.l_type = F_RDLCK;        // Read lock
    lock.l_whence = SEEK_SET;     // From beginning
    lock.l_start = 0;             // Start at offset 0
    lock.l_len = 0;               // Lock the whole file
    lock.l_pid = getpid();

    return fcntl(fd, F_SETLKW, &lock);  // Wait and acquire lock
}

int write_lock(int fd)
{
    struct flock lock;
    lock.l_type = F_WRLCK;        // Write lock
    lock.l_whence = SEEK_SET;     // From beginning
    lock.l_start = 0;             // Start at offset 0
    lock.l_len = 0;               // Lock the whole file
    lock.l_pid = getpid();

    return fcntl(fd, F_SETLKW, &lock);  // Wait and acquire lock
}

int unlock_file(int fd)
{
    struct flock lock;
    lock.l_type = F_UNLCK;        // Unlock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    return fcntl(fd, F_SETLK, &lock);
}

// load data from users.txt and courses.txt
void loadData()
{
    // load users
    FILE *file = fopen("users.txt", "r");
    if (file == NULL)
    {
        perror("Failed to open users.txt");
        return;
    }

    char line[200];
    userCount = 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (userCount >= MAX_USERS) break;

        line[strcspn(line, "\n")] = 0; // Remove newline

        char *username = strtok(line, " ");
        char *password = strtok(NULL, " ");
        char *role = strtok(NULL, " ");
        char *blockedStr = strtok(NULL, " ");

        if (username && password && role && blockedStr)
        {
            strncpy(users[userCount].username, username, sizeof(users[userCount].username) - 1);
            strncpy(users[userCount].password, password, sizeof(users[userCount].password) - 1);
            strncpy(users[userCount].role, role, sizeof(users[userCount].role) - 1);
            users[userCount].blocked = atoi(blockedStr); // Convert string to int

            users[userCount].username[sizeof(users[userCount].username) - 1] = '\0';
            users[userCount].password[sizeof(users[userCount].password) - 1] = '\0';
            users[userCount].role[sizeof(users[userCount].role) - 1] = '\0';

            userCount++;
        }
    }

    fclose(file);

    // load courses
    file = fopen("courses.txt", "r");
    if (file == NULL)
    {
        perror("Failed to open courses.txt");
        return;
    }

    courseCount = 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (courseCount >= MAX_COURSES) break;

        line[strcspn(line, "\n")] = 0; // Remove newline

        char *courseId = strtok(line, " ");
        char *courseName = strtok(NULL, " ");
        char *capStr = strtok(NULL, " ");
        char *prof = strtok(NULL, " ");
        char *enrStr = strtok(NULL, " ");

        if (courseId && courseName && capStr && prof && enrStr)
        {
            strncpy(courses[courseCount].code, courseId, sizeof(courses[courseCount].code) - 1);
            strncpy(courses[courseCount].name, courseName, sizeof(courses[courseCount].name) - 1);
            strncpy(courses[courseCount].faculty, prof, sizeof(courses[courseCount].faculty) - 1);
            courses[courseCount].capacity = atoi(capStr); // Convert string to int
            courses[courseCount].enrolled = atoi(enrStr);

            courses[courseCount].code[sizeof(courses[courseCount].code) - 1] = '\0';
            courses[courseCount].name[sizeof(courses[courseCount].name) - 1] = '\0';
            courses[courseCount].faculty[sizeof(courses[courseCount].faculty) - 1] = '\0';

            courseCount++;
        }
    }

    fclose(file);
}

// load data from enrollments.txt
void loadEnrollments()
{
    FILE *fp = fopen("enrollments.txt", "r");
    if (!fp)
    {
        perror("Failed to open enrollments file");
        return;
    }

    char line[512];
    studentCount = 0;

    while (fgets(line, sizeof(line), fp))
    {
        // Remove newline character if present
        line[strcspn(line, "\n")] = 0;

        char *token = strtok(line, " ");
        if (!token) continue;

        strncpy(students[studentCount].studentId, token, sizeof(students[studentCount].studentId));

        int courseIndex = 0;
        while ((token = strtok(NULL, " ")) && courseIndex < 10)
        {
            strncpy(students[studentCount].courseIds[courseIndex], token, 20);
            courseIndex++;
        }

        students[studentCount].n = courseIndex;
        studentCount++;
    }

    fclose(fp);
}

// login function
void login(int clientSocket)
{
    char username[50], password[50], role[50];
    recv(clientSocket, role, sizeof(role), 0);
    recv(clientSocket, username, sizeof(username), 0);
    recv(clientSocket, password, sizeof(password), 0);

    // Validate user credentials
    for (int i = 0; i < userCount; i++)
    {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0 && strcmp(users[i].role, role) == 0)
        {
            if (strcmp(users[i].role, "student") == 0 && users[i].blocked == 1)
            {
                char err[] = "User is blocked";
                send(clientSocket, err, strlen(err) + 1, 0);
                return;
            }
            // Send success message and role
            char msg[] = "Login successful";
            send(clientSocket, msg, strlen(msg) + 1, 0);  // +1 includes null terminator

            // Admin
            if (strcmp(users[i].role, "admin") == 0)
            {
                while (1)
                {
                    int choice;
                    ssize_t bytes = recv(clientSocket, &choice, sizeof(choice), 0);
                    if (bytes <= 0 || choice == 9) break;

                    if (choice == 1)
                    {
                        addStudent(clientSocket);
                        continue;
                    }
                    else if (choice == 2)
                    {
                        displayStudents(clientSocket);
                        continue;
                    }
                    else if (choice == 3)
                    {
                        addFaculty(clientSocket);
                        continue;
                    }
                    else if (choice == 4)
                    {
                        displayFaculty(clientSocket);
                        continue;
                    }
                    else if (choice == 5)
                    {
                        activateStudent(clientSocket);
                        continue;
                    }
                    else if (choice == 6)
                    {
                        blockStudent(clientSocket);
                        continue;
                    }
                    else if (choice == 7)
                    {
                        modifyStudent(clientSocket);
                        continue;
                    }
                    else if (choice == 8)
                    {
                        modifyFaculty(clientSocket);
                        continue;
                    }
                    else
                    {
                        char invalid[] = "Invalid choice.";
                        send(clientSocket, invalid, strlen(invalid) + 1, 0);
                    }
                }
            }

            // Faculty
            else if (strcmp(users[i].role, "faculty") == 0)
            {
                while (1)
                {
                    int choice;
                    ssize_t bytes = recv(clientSocket, &choice, sizeof(choice), 0);
                    if (bytes <= 0 || choice == 7) break;

                    if (choice == 1)
                    {
                        viewCourses(users[i].username, clientSocket);
                        continue;
                    }
                    else if (choice == 2)
                    {
                        addCourse(users[i].username, clientSocket);
                        continue;
                    }
                    else if (choice == 3)
                    {
                        removeCourse(users[i].username, clientSocket);
                        continue;
                    }
                    else if (choice == 4)
                    {
                        updateCourse(users[i].username, clientSocket);
                        continue;
                    }
                    else if (choice == 5)
                    {
                        viewEnrollments(clientSocket);
                        continue;
                    }
                    else if (choice == 6)
                    {
                        char pass[20];
                        recv(clientSocket, pass, sizeof(pass), 0);
                        strncpy(users[i].password, pass, sizeof(users[i].password) - 1);
                        users[i].password[sizeof(users[i].password) - 1] = '\0';
                        updateUsers();
                        char response[50];
                        strcpy(response, "Password changed successfully");
                        send(clientSocket, response, strlen(response) + 1, 0);
                        continue;
                    }
                    else
                    {
                        char invalid[] = "Invalid choice.";
                        send(clientSocket, invalid, strlen(invalid) + 1, 0);
                    }
                }
            }

            // Student
            else if (strcmp(users[i].role, "student") == 0)
            {
                while (1)
                {
                    int choice;
                    ssize_t bytes = recv(clientSocket, &choice, sizeof(choice), 0);
                    if (bytes <= 0 || choice == 6) break;

                    if (choice == 1)
                    {
                        viewAllCourses(clientSocket);
                        continue;
                    }
                    else if (choice == 2)
                    {
                        enroll(users[i].username, clientSocket);
                        continue;
                    }
                    else if (choice == 3)
                    {
                        drop(users[i].username, clientSocket);
                        continue;
                    }
                    else if (choice == 4)
                    {
                        viewEnrolled(users[i].username, clientSocket);
                        continue;
                    }
                    else if (choice == 5)
                    {
                        char pass[20];
                        recv(clientSocket, pass, sizeof(pass), 0);
                        strncpy(users[i].password, pass, sizeof(users[i].password) - 1);
                        users[i].password[sizeof(users[i].password) - 1] = '\0';
                        updateUsers();
                        char response[50];
                        strcpy(response, "Password changed successfully");
                        send(clientSocket, response, strlen(response) + 1, 0);
                        continue;
                    }
                    else
                    {
                        char invalid[] = "Invalid choice.";
                        send(clientSocket, invalid, strlen(invalid) + 1, 0);
                    }
                }
            }

            return;
        }
    }
    // Send failure message
    send(clientSocket, "Invalid credentials", 20, 0);
}

// save new user details to users.txt
void saveUserToFile(User *user)
{
    int fd = open("users.txt", O_WRONLY | O_APPEND);
    if (write_lock(fd) == -1)
    {
        perror("Failed to acquire write lock");
        close(fd);
        return;
    }

    char buffer[200];
    int len = snprintf(buffer, sizeof(buffer), "%s %s %s %d\n", user->username, user->password, user->role, user->blocked);
    
    if (write(fd, buffer, len) < 0)
    {
        perror("Failed to write to file");
    }

    unlock_file(fd);
    close(fd);
}

// Rewrite users.txt with the updated users array.
void updateUsers()
{
    int fd = open("users.txt", O_WRONLY | O_TRUNC);
    if (fd < 0)
    {
        perror("Error opening users.txt for update");
        return;
    }

    if (write_lock(fd) == -1)
    {
        perror("Failed to acquire write lock");
        close(fd);
        return;
    }

    char line[256];
    for (int i = 0; i < userCount; i++)
    {
        // Prepare each record; increasing buffer size to handle additional data.
        int len = snprintf(line, sizeof(line), "%s %s %s %d\n",
                           users[i].username,
                           users[i].password,
                           users[i].role,
                           users[i].blocked);
        if (len < 0)
        {
            perror("snprintf error");
            continue;
        }
        if (write(fd, line, len) < 0)
        {
            perror("Error writing to file");
        }
    }

    unlock_file(fd);
    close(fd);
}

// function to add a new student
void addStudent(int clientSocket)
{
    char username[50], password[50];
    char buffer[100];

    recv(clientSocket, username, sizeof(username), 0);
    recv(clientSocket, password, sizeof(password), 0);

    // Check if username already exists
    for (int i = 0; i < userCount; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            strcpy(buffer, "Username already exists. Cannot add student.");
            send(clientSocket, buffer, strlen(buffer)+1, 0);
            return;
        }
    }

    // Add new student to users array
    if (userCount >= MAX_USERS)
    {
        strcpy(buffer, "User limit reached. Cannot add student.");
        send(clientSocket, buffer, strlen(buffer)+1, 0);
        return;
    }

    User newStudent;
    strncpy(newStudent.username, username, sizeof(newStudent.username)-1);
    strncpy(newStudent.password, password, sizeof(newStudent.password)-1);
    strncpy(newStudent.role, "student", sizeof(newStudent.role)-1);
    newStudent.username[sizeof(newStudent.username)-1] = '\0';
    newStudent.password[sizeof(newStudent.password)-1] = '\0';
    newStudent.role[sizeof(newStudent.role)-1] = '\0';
    newStudent.blocked = 0;
    users[userCount++] = newStudent;
    saveUserToFile(&newStudent);
    strcpy(buffer, "Student added successfully.");
    send(clientSocket, buffer, strlen(buffer)+1, 0);
}

// // function to block a student
void blockStudent(int clientSocket)
{
    char username[50], response[100];
    recv(clientSocket, username, sizeof(username), 0);

    // Search for the user with the given username who is a student
    int found = 0;
    for (int i = 0; i < userCount; i++)
    {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].role, "student") == 0)
        {
            // Set the blocked flag to 1
            users[i].blocked = 1;
            found = 1;
            break;
        }
    }

    if (!found)
    {
        strcpy(response, "Student not found");
        send(clientSocket, response, strlen(response) + 1, 0);
        return;
    }

    updateUsers();
    strcpy(response, "Student blocked successfully");
    send(clientSocket, response, strlen(response) + 1, 0);
}

// function to activate a student
void activateStudent(int clientSocket)
{
    char username[50], response[100];
    recv(clientSocket, username, sizeof(username), 0);

    // Search for the user with the given username who is a student
    int found = 0;
    for (int i = 0; i < userCount; i++)
    {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].role, "student") == 0)
        {
            // Set the blocked flag to 1
            users[i].blocked = 0;
            found = 1;
            break;
        }
    }

    if (!found)
    {
        strcpy(response, "Student not found");
        send(clientSocket, response, strlen(response) + 1, 0);
        return;
    }

    updateUsers();
    strcpy(response, "Student activated successfully");
    send(clientSocket, response, strlen(response) + 1, 0);
}


// function to modify student details
void modifyStudent(int clientSocket)
{
    char username[50], newuser[50], password[50], role[50], response[100];
    recv(clientSocket, username, sizeof(username), 0);
    recv(clientSocket, newuser, sizeof(newuser), 0);
    recv(clientSocket, password, sizeof(password), 0);
    recv(clientSocket, role, sizeof(role), 0);

    // Search for the user with the given username who is a student
    int found = 0;
    for (int i = 0; i < userCount; i++)
    {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].role, "student") == 0)
        {
            strncpy(users[i].username, newuser, sizeof(users[i].username) - 1);
            strncpy(users[i].password, password, sizeof(users[i].password) - 1);
            strncpy(users[i].role, role, sizeof(users[i].role) - 1);
            users[i].username[sizeof(users[i].username) - 1] = '\0';
            users[i].password[sizeof(users[i].password) - 1] = '\0';
            users[i].role[sizeof(users[i].role) - 1] = '\0';
            found = 1;
            break;
        }
    }

    if (!found)
    {
        strcpy(response, "Student not found");
        send(clientSocket, response, strlen(response) + 1, 0);
        return;
    }

    updateUsers();
    strcpy(response, "Student details modified successfully");
    send(clientSocket, response, strlen(response) + 1, 0);
    return;
}

// function to display student details
void displayStudents(int clientSocket)
{
    int fd = open("users.txt", O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening file");
        return;
    }

    if (read_lock(fd) == -1)
    {
        perror("Failed to acquire read lock");
        close(fd);
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = read(fd, buffer, BUFFER_SIZE - 1);

    if (bytesRead < 0)
    {
        perror("Error reading file");
        close(fd);
        return;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the buffer

    // Parse and display student details only
    char *line = strtok(buffer, "\n");
    char send_buffer[BUFFER_SIZE] = "";
    
    while (line != NULL)
    {
        char username[50], password[50], role[50];
        int blocked;
        if (sscanf(line, "%s %s %s %d", username, password, role, &blocked) == 4)
        {
            if (strcmp(role, "student") == 0)
            {
                char temp[512];
                snprintf(temp, sizeof(temp), "Username: %s\nPassword: %s\nRole: %s\nBlocked: %s\n-----------------------------\n", username, password, role, blocked ? "Yes" : "No");
                strncat(send_buffer, temp, sizeof(send_buffer) - strlen(send_buffer) - 1);
            }
        }
        line = strtok(NULL, "\n");
    }
    send(clientSocket, send_buffer, strlen(send_buffer), 0);
    unlock_file(fd);
    close(fd);
}

// function to add a new faculty
void addFaculty(int clientSocket)
{
    char username[50], password[50];
    char buffer[100];

    recv(clientSocket, username, sizeof(username), 0);
    recv(clientSocket, password, sizeof(password), 0);

    // Check if username already exists
    for (int i = 0; i < userCount; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            strcpy(buffer, "Username already exists. Cannot add faculty.");
            send(clientSocket, buffer, strlen(buffer)+1, 0);
            return;
        }
    }

    // Add new student to users array
    if (userCount >= MAX_USERS)
    {
        strcpy(buffer, "User limit reached. Cannot add faculty.");
        send(clientSocket, buffer, strlen(buffer)+1, 0);
        return;
    }

    User newFaculty;
    strncpy(newFaculty.username, username, sizeof(newFaculty.username)-1);
    strncpy(newFaculty.password, password, sizeof(newFaculty.password)-1);
    strncpy(newFaculty.role, "faculty", sizeof(newFaculty.role)-1);
    newFaculty.username[sizeof(newFaculty.username)-1] = '\0';
    newFaculty.password[sizeof(newFaculty.password)-1] = '\0';
    newFaculty.role[sizeof(newFaculty.role)-1] = '\0';
    newFaculty.blocked = 0;
    users[userCount++] = newFaculty;
    saveUserToFile(&newFaculty);
    strcpy(buffer, "Faculty added successfully.");
    send(clientSocket, buffer, strlen(buffer)+1, 0);
}

// function to display faculty details
void displayFaculty(int clientSocket)
{
    int fd = open("users.txt", O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening file");
        return;
    }

    if (read_lock(fd) == -1)
    {
        perror("Failed to acquire read lock");
        close(fd);
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = read(fd, buffer, BUFFER_SIZE - 1);

    if (bytesRead < 0)
    {
        perror("Error reading file");
        close(fd);
        return;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the buffer

    // Parse and display faculty details only
    char *line = strtok(buffer, "\n");
    char send_buffer[BUFFER_SIZE] = "";
    
    while (line != NULL)
    {
        char username[50], password[50], role[50];
        if (sscanf(line, "%s %s %s", username, password, role) == 3)
        {
            if (strcmp(role, "faculty") == 0)
            {
                char temp[512];
                snprintf(temp, sizeof(temp), "Username: %s\nPassword: %s\nRole: %s\n-----------------------------\n", username, password, role);
                strncat(send_buffer, temp, sizeof(send_buffer) - strlen(send_buffer) - 1);
            }
        }
        line = strtok(NULL, "\n");
    }
    send(clientSocket, send_buffer, strlen(send_buffer), 0);
    unlock_file(fd);
    close(fd);
}

// function to modify faculty details
void modifyFaculty(int clientSocket)
{
    char username[50], newuser[50], password[50], role[50], response[100];
    recv(clientSocket, username, sizeof(username), 0);
    recv(clientSocket, newuser, sizeof(newuser), 0);
    recv(clientSocket, password, sizeof(password), 0);
    recv(clientSocket, role, sizeof(role), 0);

    // Search for the user with the given username who is a student
    int found = 0;
    for (int i = 0; i < userCount; i++)
    {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].role, "faculty") == 0)
        {
            strncpy(users[i].username, newuser, sizeof(users[i].username) - 1);
            strncpy(users[i].password, password, sizeof(users[i].password) - 1);
            strncpy(users[i].role, role, sizeof(users[i].role) - 1);
            users[i].username[sizeof(users[i].username) - 1] = '\0';
            users[i].password[sizeof(users[i].password) - 1] = '\0';
            users[i].role[sizeof(users[i].role) - 1] = '\0';
            found = 1;
            break;
        }
    }

    if (!found)
    {
        strcpy(response, "Faculty not found");
        send(clientSocket, response, strlen(response) + 1, 0);
        return;
    }

    updateUsers();
    strcpy(response, "Faculty details modified successfully");
    send(clientSocket, response, strlen(response) + 1, 0);
    return;
}

// function to add a new course to courses.txt
void saveCourseToFile(Course *course)
{
    int fd = open("courses.txt", O_WRONLY | O_APPEND);
    if (write_lock(fd) == -1)
    {
        perror("Failed to acquire write lock");
        close(fd);
        return;
    }

    char buffer[200];
    int len = snprintf(buffer, sizeof(buffer), "%s %s %d %s %d\n", course->code, course->name, course->capacity, course->faculty, course->enrolled);
    
    if (write(fd, buffer, len) < 0)
    {
        perror("Failed to write to file");
    }

    unlock_file(fd);
    close(fd);
}

// function to add a new course
void addCourse(char *instructor, int clientSocket)
{
    char course_code[10], course_name[20], buffer[100];
    int cap;
    recv(clientSocket, course_code, sizeof(course_code), 0);
    recv(clientSocket, course_name, sizeof(course_name), 0);
    recv(clientSocket, &cap, sizeof(cap), 0);

    // Check if username already exists
    for (int i = 0; i < courseCount; i++)
    {
        if (strcmp(courses[i].code, course_code) == 0)
        {
            strcpy(buffer, "Duplicate course code. Cannot add course.");
            send(clientSocket, buffer, strlen(buffer)+1, 0);
            return;
        }
    }

    Course newCourse;
    strncpy(newCourse.code, course_code, sizeof(newCourse.code)-1);
    strncpy(newCourse.name, course_name, sizeof(newCourse.name)-1);
    strncpy(newCourse.faculty, instructor, sizeof(newCourse.faculty)-1);
    newCourse.code[sizeof(newCourse.code)-1] = '\0';
    newCourse.name[sizeof(newCourse.name)-1] = '\0';
    newCourse.faculty[sizeof(newCourse.faculty)-1] = '\0';
    newCourse.capacity = cap;
    newCourse.enrolled = 0;
    courses[courseCount++] = newCourse;
    saveCourseToFile(&newCourse);
    strcpy(buffer, "Course added successfully.");
    send(clientSocket, buffer, strlen(buffer)+1, 0);
}

// function to view courses offered by a particular prof
void viewCourses(char *instructor, int clientSocket)
{
    int fd = open("courses.txt", O_RDONLY);
    if (fd == -1)
    {
        char error_msg[] = "Could not open courses.txt\n";
        send(clientSocket, error_msg, strlen(error_msg), 0);
        return;
    }

    if (read_lock(fd) == -1)
    {
        perror("Failed to acquire read lock");
        close(fd);
        return;
    }

    char file_buffer[BUFFER_SIZE];
    char result_buffer[BUFFER_SIZE] = {0};
    int bytes_read, offset = 0;

    while ((bytes_read = read(fd, file_buffer, sizeof(file_buffer) - 1)) > 0)
    {
        file_buffer[bytes_read] = '\0';
        char *line = strtok(file_buffer, "\n");
        while (line)
        {
            char code[20], name[50], prof[50];
            int cap, enr;
            if (sscanf(line, "%s %s %d %s %d", code, name, &cap, prof, &enr) == 5)
            {
                if (strcmp(prof, instructor) == 0)
                {
                    offset += snprintf(result_buffer + offset, sizeof(result_buffer) - offset, "Course Code: %s, Course Name: %s, Course Capacity: %d, Currently Enrolled: %d\n", code, name, cap, enr);
                }
            }
            line = strtok(NULL, "\n");
        }
    }

    unlock_file(fd);
    close(fd);

    if (strlen(result_buffer) == 0)
    {
        strcpy(result_buffer, "No courses found for this faculty.\n");
    }

    send(clientSocket, result_buffer, strlen(result_buffer), 0);
}

// function to view enrollments for a course
void viewEnrollments(int clientSocket)
{
    char courseCode[10] = {0};
    ssize_t len = recv(clientSocket, courseCode, sizeof(courseCode) - 1, 0);
    if (len <= 0) return;
    courseCode[len] = '\0'; // Ensure null-termination

    // Validate course code
    int found = 0;
    int enrolledCount = 0;
    char buffer[2048] = {0};
    for (int i = 0; i < courseCount; i++)
    {
        if (strcmp(courses[i].code, courseCode) == 0)
        {
            found = 1;
            enrolledCount = courses[i].enrolled;
            break;
        }
    }

    if (!found)
    {
        int offset = snprintf(buffer, sizeof(buffer), "Invalid course code\n\n");
        buffer[sizeof(buffer) - 1] = '\0';
        send(clientSocket, buffer, strlen(buffer) + 1, 0);
        return;
    }

    if (enrolledCount == 0)
    {
        int offset = snprintf(buffer, sizeof(buffer), "No enrollments for this course yet\n\n");
        buffer[sizeof(buffer) - 1] = '\0';
        send(clientSocket, buffer, strlen(buffer) + 1, 0);
        return;
    }

    int offset = snprintf(buffer, sizeof(buffer),
        "The following students are enrolled in this course:\n\n");

    for (int i = 0; i < studentCount; i++)
    {
        for (int j = 0; j < students[i].n; j++)
        {
            if (strcmp(students[i].courseIds[j], courseCode) == 0)
            {
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                    "%s\n", students[i].studentId);
                break;
            }
        }
    }

    buffer[sizeof(buffer) - 1] = '\0';
    send(clientSocket, buffer, strlen(buffer) + 1, 0);
}

// function to rewrite courses.txt with the updated courses array
void updateCourseFile()
{
    // Rewrite file with updated courses
    int fd = open("courses.txt", O_WRONLY | O_TRUNC);
    if (fd == -1)
    {
        perror("Failed to open courses.txt for writing");
        return;
    }

    if (write_lock(fd) == -1)
    {
        perror("Failed to acquire write lock");
        close(fd);
        return;
    }

    char line[256];

    for (int i = 0; i < courseCount; i++)
    {
        int len = snprintf(line, sizeof(line), "%s %s %d %s %d\n",
                           courses[i].code,
                           courses[i].name,
                           courses[i].capacity,
                           courses[i].faculty,
                           courses[i].enrolled);
        if (len < 0)
        {
            perror("snprintf error");
            continue;
        }
        if (write(fd, line, len) < 0)
        {
            perror("Error writing to file");
        }
    }

    unlock_file(fd);
    close(fd);
}

// function to delete a course
void removeCourse(char *instructor, int clientSocket)
{
    char courseCode[10];
    recv(clientSocket, courseCode, sizeof(courseCode), 0);
    int found = 0;

    for (int i = 0; i < courseCount; i++)
    {
        if (strcmp(courses[i].code, courseCode) == 0 && strcmp(courses[i].faculty, instructor) == 0)
        {
            found = 1;

            // Shift remaining elements left
            for (int j = i; j < courseCount - 1; j++)
            {
                courses[j] = courses[j + 1];
            }

            courseCount--;  // Reduce total course count
            break;
        }
    }

    char response[50];

    if (!found)
    {
        strcpy(response, "Course not found");
        send(clientSocket, response, strlen(response) + 1, 0);
        return;
    }

    updateCourseFile();

    strcpy(response, "Course succesfully removed");
    send(clientSocket, response, strlen(response) + 1, 0);
    return;
}

// function to update course details
void updateCourse(char *instructor, int clientSocket)
{
    char courseCode[10], newCourseName[20], response[50];
    int newCapacity;
    recv(clientSocket, courseCode, sizeof(courseCode), 0);
    recv(clientSocket, newCourseName, sizeof(newCourseName), 0);
    recv(clientSocket, &newCapacity, sizeof(newCapacity), 0);

    int found = 0;
    for (int i = 0; i < courseCount; i++)
    {
        if (strcmp(courses[i].code, courseCode) == 0 && strcmp(courses[i].faculty, instructor) == 0)
        {
            strncpy(courses[i].name, newCourseName, sizeof(courses[i].name) - 1);
            courses[i].name[sizeof(courses[i].name) - 1] = '\0';
            courses[i].capacity = newCapacity;
            found = 1;
            break;
        }
    }

    if (!found)
    {
        strcpy(response, "Course not found");
        send(clientSocket, response, strlen(response) + 1, 0);
        return;
    }

    updateCourseFile();
    strcpy(response, "Course details updated successfully");
    send(clientSocket, response, strlen(response) + 1, 0);
    return;
}

// function to view all available courses
void viewAllCourses(int clientSocket)
{
    int fd = open("courses.txt", O_RDONLY);
    if (fd == -1)
    {
        char error_msg[] = "Could not open courses.txt\n";
        send(clientSocket, error_msg, strlen(error_msg), 0);
        return;
    }

    if (read_lock(fd) == -1)
    {
        perror("Failed to acquire read lock");
        close(fd);
        return;
    }

    char file_buffer[BUFFER_SIZE];
    char result_buffer[BUFFER_SIZE] = {0};
    int bytes_read, offset = 0;

    while ((bytes_read = read(fd, file_buffer, sizeof(file_buffer) - 1)) > 0)
    {
        file_buffer[bytes_read] = '\0';
        char *line = strtok(file_buffer, "\n");
        while (line)
        {
            char code[20], name[50], prof[50];
            int cap, enr;
            if (sscanf(line, "%s %s %d %s %d", code, name, &cap, prof, &enr) == 5)
            {
                offset += snprintf(result_buffer + offset, sizeof(result_buffer) - offset, "Course Code: %s, Course Name: %s, Course Capacity: %d, Professor: %s, Currently Enrolled: %d\n", code, name, cap, prof, enr);
            }
            line = strtok(NULL, "\n");
        }
    }

    unlock_file(fd);
    close(fd);
    send(clientSocket, result_buffer, strlen(result_buffer), 0);
}

// function to rewrite enrollments.txt with the updated array
void saveEnrollments()
{
    int fd = open("enrollments.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("Failed to open file for writing");
        return;
    }

    if (write_lock(fd) == -1)
    {
        perror("Failed to acquire write lock");
        close(fd);
        return;
    }

    char line[512];
    for (int i = 0; i < studentCount; i++)
    {
        int len = snprintf(line, sizeof(line), "%s", students[i].studentId);
        for (int j = 0; j < students[i].n; j++)
        {
            len += snprintf(line + len, sizeof(line) - len, " %s", students[i].courseIds[j]);
        }
        len += snprintf(line + len, sizeof(line) - len, "\n");

        if (write(fd, line, len) == -1)
        {
            perror("Write failed");
            unlock_file(fd);
            close(fd);
            return;
        }
    }

    unlock_file(fd);
    close(fd);
}

// function to enroll in a new course
void enroll(char *studentId, int clientSocket)
{
    char courseId[10], response[100];
    recv(clientSocket, courseId, sizeof(courseId), 0);

    // Check if course exists
    int courseCap = 0;
    int curr_enr = 0;
    Course *course;

    for (int i = 0; i < courseCount; i++)
    {
        if (strcmp(courses[i].code, courseId) == 0)
        {
            courseCap = courses[i].capacity;
            curr_enr = courses[i].enrolled;
            course = &courses[i];
        }
    }

    if (courseCap == 0)
    {
        strcpy(response, "Invalid course code");
        send(clientSocket, response, strlen(response) + 1, 0);
        return;
    }

    else if (courseCap - curr_enr == 0)
    {
        strcpy(response, "Course is full");
        send(clientSocket, response, strlen(response) + 1, 0);
        return;
    }

    int found = 0;
    for (int i = 0; i < studentCount; i++)
    {
        if (strcmp(students[i].studentId, studentId) == 0)
        {
            found = 1;

            // Check if course already enrolled
            for (int j = 0; j < students[i].n; j++)
            {
                if (strcmp(students[i].courseIds[j], courseId) == 0)
                {
                    strcpy(response, "You're already enrolled in this course");
                    send(clientSocket, response, strlen(response) + 1, 0);
                    return;
                }
            }

            // Enroll in course
            if (students[i].n < 10)
            {
                strncpy(students[i].courseIds[students[i].n], courseId, 20);
                students[i].n++;
                strcpy(response, "Enrolled successfully");
                send(clientSocket, response, strlen(response) + 1, 0);
            }
            else
            {
                strcpy(response, "You have reached the max no. of courses");
                send(clientSocket, response, strlen(response) + 1, 0);
                return;
            }
            break;
        }
    }

    // New student
    if (!found)
    {
        strncpy(students[studentCount].studentId, studentId, 20);
        strncpy(students[studentCount].courseIds[0], courseId, 20);
        students[studentCount].n = 1;
        studentCount++;
        strcpy(response, "Enrolled successfully");
        send(clientSocket, response, strlen(response) + 1, 0);
    }

    course->enrolled += 1;
    updateCourseFile();
    saveEnrollments();
}

// function to drop a course
void drop(char *studentId, int clientSocket)
{
    char courseId[10], response[100];
    recv(clientSocket, courseId, sizeof(courseId), 0);

    int foundStudent = 0, foundCourse = 0;

    for (int i = 0; i < studentCount; i++)
    {
        if (strcmp(students[i].studentId, studentId) == 0)
        {
            foundStudent = 1;

            for (int j = 0; j < students[i].n; j++)
            {
                if (strcmp(students[i].courseIds[j], courseId) == 0)
                {
                    // Shift the remaining courses left
                    for (int k = j; k < students[i].n - 1; k++)
                    {
                        strcpy(students[i].courseIds[k], students[i].courseIds[k + 1]);
                    }
                    students[i].n--;
                    foundCourse = 1;
                    break;
                }
            }
            break;
        }
    }

    if (!foundStudent || !foundCourse)
    {
        strcpy(response, "You are not enrolled in this course");
        send(clientSocket, response, strlen(response) + 1, 0);
        return;
    }

    for (int i = 0; i < courseCount; i++)
    {
        if (strcmp(courses[i].code, courseId) == 0)
        {
            courses[i].enrolled --;
            break;
        }
    }
    
    updateCourseFile();
    strcpy(response, "Course dropped successfully");
    send(clientSocket, response, strlen(response) + 1, 0);
    saveEnrollments();
}

// function to view enrolled course details
void viewEnrolled(char *studentId, int clientSocket)
{
    int found = -1;
    for (int i = 0; i < studentCount; i++)
    {
        if (strcmp(students[i].studentId, studentId) == 0)
        {
            if (students[i].n != 0)
            {
                found = i;
                break;
            }
        }
    }

    if (found == -1)
    {
        char msg[] = "You are not enrolled in any courses\n";
        send(clientSocket, msg, strlen(msg), 0);
        return;
    }

    char buffer[2048] = "";
    snprintf(buffer, sizeof(buffer), "You are enrolled in these courses:\n\n");

    for (int i = 0; i < students[found].n; i++)
    {
        char *cid = students[found].courseIds[i];
        int match = 0;

        for (int j = 0; j < courseCount; j++)
        {
            if (strcmp(courses[j].code, cid) == 0)
            {
                char line[256];
                snprintf(line, sizeof(line), "Course Code: %s, Course Name: %s, Capacity: %d, Faculty: %s, Currently Enrolled: %d\n",
                         courses[j].code, courses[j].name,
                         courses[j].capacity, courses[j].faculty, courses[j].enrolled);
                strncat(buffer, line, sizeof(buffer) - strlen(buffer) - 1);
                match = 1;
                break;
            }
        }

        if (!match)
        {
            char line[100];
            snprintf(line, sizeof(line), "Course details not found");
            strncat(buffer, line, sizeof(buffer) - strlen(buffer) - 1);
        }
    }

    send(clientSocket, buffer, strlen(buffer), 0);
}

void *handleClient(void *clientSocket)
{
    int sock = *(int*)clientSocket;
    free(clientSocket);
    login(sock);
    close(sock);
    return NULL;
}

int main()
{
    loadData(); // Load existing user data
    loadEnrollments();

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 10);
    printf("Server listening on port 8080...\n");
    while (1) {
        addr_size = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
        printf("Client connected.\n");
        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        *pclient = clientSocket;
        pthread_create(&tid, NULL, handleClient, pclient);
        pthread_detach(tid);
    }

    close(serverSocket);
    return 0;
}