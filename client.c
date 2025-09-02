#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main()
{
    int sock;
    struct sockaddr_in serverAddr;
    char username[50], password[50], response[50], role[50];
    int role_no;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    printf("\n..............Welcome Back to Academia :: Course Registration..............\n");
    printf("\nEnter Your Choice { 1. Admin, 2. Professor, 3. Student } : ");
    scanf("%d", &role_no);

    if (role_no == 1) strcpy(role, "admin");
    else if (role_no == 2) strcpy(role, "faculty");
    else if (role_no == 3) strcpy(role, "student");
    else
    {
        printf("Invalid choice.\n");
        close(sock);
        return 0;
    }

    // Input username and password
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);

    // Send role, username and password to server
    send(sock, role, sizeof(role), 0);
    send(sock, username, sizeof(username), 0);
    send(sock, password, sizeof(password), 0);

    // Receive response from server
    memset(response, 0, sizeof(response));
    recv(sock, response, sizeof(response), 0);
    printf("%s\n", response);

    if (strcmp(response, "Login successful") != 0)
    {
        close(sock);
        return 0;
    }

    // Admin
    if (role_no == 1)
    {
        while (1)
        {
            int choice;
            printf("\n........Welcome to Admin Menu........\n");
            printf("1. Add Student\n2. View Student Details\n3. Add Faculty\n4. View Faculty Deatails\n5. Activate Student\n6. Block Student\n7. Modify Student Details\n8. Modify Faculty Details\n9. Logout and Exit\n");
            printf("Enter choice: ");
            scanf("%d", &choice);
            printf("\n");
            send(sock, &choice, sizeof(choice), 0);
            
            if (choice == 9)
            {
                printf("Logged out successfully\n");
                break;
            }
            
            else if (choice == 1)
            {
                printf("Enter new student's username: ");
                scanf("%s", username);
                printf("Enter new student's password: ");
                scanf("%s", password);
                send(sock, username, sizeof(username), 0);
                send(sock, password, sizeof(password), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else if (choice == 2)
            {
                char buffer[BUFFER_SIZE] = {0};
                read(sock, buffer, BUFFER_SIZE - 1);
                printf("Received student details from server:\n\n%s", buffer);
                continue;
            }

            else if (choice == 3)
            {
                printf("Enter new faculty's username: ");
                scanf("%s", username);
                printf("Enter new faculty's password: ");
                scanf("%s", password);
                send(sock, username, sizeof(username), 0);
                send(sock, password, sizeof(password), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else if (choice == 4)
            {
                char buffer[BUFFER_SIZE] = {0};
                read(sock, buffer, BUFFER_SIZE - 1);
                printf("Received faculty details from server:\n\n%s", buffer);
                continue;
            }

            else if (choice == 5)
            {
                printf("Enter student's username to activate: ");
                scanf("%s", username);
                send(sock, username, sizeof(username), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else if (choice == 6)
            {
                printf("Enter student's username to block: ");
                scanf("%s", username);
                send(sock, username, sizeof(username), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }
            
            else if (choice == 7)
            {
                char newuser[50];
                printf("Enter student's username to modify: ");
                scanf("%s", username);
                printf("Enter new username: ");
                scanf("%s", newuser);
                printf("Enter new password: ");
                scanf("%s", password);
                printf("Enter new role: ");
                scanf("%s", role);
                send(sock, username, sizeof(username), 0);
                send(sock, newuser, sizeof(newuser), 0);
                send(sock, password, sizeof(password), 0);
                send(sock, role, sizeof(role), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else if (choice == 8)
            {
                char newuser[50];
                printf("Enter faculty username to modify: ");
                scanf("%s", username);
                printf("Enter new username: ");
                scanf("%s", newuser);
                printf("Enter new password: ");
                scanf("%s", password);
                printf("Enter new role: ");
                scanf("%s", role);
                send(sock, username, sizeof(username), 0);
                send(sock, newuser, sizeof(newuser), 0);
                send(sock, password, sizeof(password), 0);
                send(sock, role, sizeof(role), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else
            {
                printf("Invalid choice. Try again.");
            }
        }
    }

    else if (role_no == 2)
    {
        while (1)
        {
            int choice, cap;
            char course_code[10], course_name[20];
            printf("\n........Welcome to Faculty Menu........\n");
            printf("1. View Offering Courses\n2. Add New Course\n3. Remove Course from Catalog\n4. Update Course Details\n5. View Enrollments\n6. Change Password\n7. Logout and Exit\n");
            printf("Enter choice: ");
            scanf("%d", &choice);
            printf("\n");
            send(sock, &choice, sizeof(choice), 0);
            
            if (choice == 7)
            {
                printf("Logged out successfully\n");
                break;
            }

            else if (choice == 1)
            {
                char buffer[BUFFER_SIZE] = {0};
                read(sock, buffer, BUFFER_SIZE - 1);
                printf("You are offering these courses:\n\n%s", buffer);
                continue;
            }

            else if (choice == 2)
            {
                printf("Enter new course code: ");
                scanf("%s", course_code);
                printf("Enter course name: ");
                scanf("%s", course_name);
                printf("Enter course capacity: ");
                scanf("%d", &cap);
                send(sock, course_code, sizeof(course_code), 0);
                send(sock, course_name, sizeof(course_name), 0);
                send(sock, &cap, sizeof(cap), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else if (choice == 3)
            {
                printf("Enter course code to be removed: ");
                scanf("%s", course_code);
                send(sock, course_code, sizeof(course_code), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else if (choice == 4)
            {
                printf("Enter course code to be updated: ");
                scanf("%s", course_code);
                printf("Enter new course name: ");
                scanf("%s", course_name);
                printf("Enter new course capacity: ");
                scanf("%d", &cap);
                send(sock, course_code, sizeof(course_code), 0);
                send(sock, course_name, sizeof(course_name), 0);
                send(sock, &cap, sizeof(cap), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else if (choice == 5)
            {
                printf("Enter course code to view enrollments: ");
                scanf("%s", course_code);
                send(sock, course_code, sizeof(course_code), 0);
                char buffer[BUFFER_SIZE] = {0};
                read(sock, buffer, BUFFER_SIZE - 1);
                printf("%s", buffer);
                continue;
            }

            else if (choice == 6)
            {
                char newpass[20];
                printf("Enter new password: ");
                scanf("%s", newpass);
                send(sock, newpass, sizeof(newpass), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else
            {
                printf("Invalid choice. Try again.");
            }

        }
    }

    else if (role_no == 3)
    {
        while (1)
        {
            int choice;
            printf("\n........Welcome to Student Menu........\n");
            printf("1. View All Courses\n2. Enroll (pick) New Course\n3. Drop Course\n4. View Enrolled Course Deatails\n5. Change Password\n6. Logout and Exit\n");
            printf("Enter choice: ");
            scanf("%d", &choice);
            printf("\n");
            send(sock, &choice, sizeof(choice), 0);
            
            if (choice == 6)
            {
                printf("Logged out successfully\n");
                break;
            }

            else if (choice == 1)
            {
                char buffer[BUFFER_SIZE] = {0};
                read(sock, buffer, BUFFER_SIZE - 1);
                printf("Available courses:\n\n%s", buffer);
                continue;
            }
            
            else if (choice == 2)
            {
                char course_code[10];
                printf("Enter course code: ");
                scanf("%s", course_code);
                send(sock, course_code, sizeof(course_code), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else if (choice == 3)
            {
                char course_code[10];
                printf("Enter course code: ");
                scanf("%s", course_code);
                send(sock, course_code, sizeof(course_code), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else if (choice == 4)
            {
                char buffer[BUFFER_SIZE] = {0};
                read(sock, buffer, BUFFER_SIZE - 1);
                printf("%s", buffer);
                continue;
            }

            else if (choice == 5)
            {
                char newpass[20];
                printf("Enter new password: ");
                scanf("%s", newpass);
                send(sock, newpass, sizeof(newpass), 0);
                recv(sock, response, sizeof(response), 0);
                printf("%s\n", response);
                continue;
            }

            else
            {
                printf("Invalid choice. Try again.");
            }
        }
    }

    close(sock);
    return 0;
}