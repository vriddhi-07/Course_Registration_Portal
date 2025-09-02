# Problem Statement 
The project aims to develop an Academia Portal that is user-friendly and multifunctional. It supports 
the following roles - Faculty, Student, Admin. The application possesses password-protected account 
login functionality, thus preventing the whole management system from unauthorized access. 
  
# Role Specific Features 
After connecting to the server and successfully logging in, clients will get the following menu options 
based on their roles. 
  
**Admin** 
1) Add Student 
2) View Student Details 
3) Add Faculty 
4) View Faculty Details 
5) Activate Student 
6) Block Student 
7) Modify Student Details 
8) Modify Faculty Details 
9) Logout and Exit
  
**Faculty** 
1) View Offering Courses 
2) Add New Course 
3) Remove Course from Catalog 
4) Update Course Details 
5) View Enrollments 
6) Change Password 
7) Logout and Exit
   
**Student** 
1) View All Courses 
2) Enroll in a New Course 
3) Drop Course 
4) View Enrolled Course Details 
5) Change Password 
6) Logout and Exit
   
# Implementation 
  
● **Socket Programming** – Server maintains the database and serves multiple clients concurrently. 
Clients can connect to the server and access their specific academic details.  
  
● All student and faculty details and course information have been stored in text files.  
  
● Read and write locks have been used to protect the critical data sections in files and to 
maintain data integrity.  
  
● System calls have been used instead of library functions to implement fundamental Operating 
Systems concepts like Process Management, File Management, File Locking, Multithreading 
and Inter Process Communication Mechanisms.  
  
# Steps to Run 
  
● Open the terminal in the project directory and compile the source code.  
➔  To compile server.c : gcc -o server -pthread server.c   
➔ To compile client.c : gcc -o client client.c  
  
● Run the server by executing: ./server  
  
● In a separate terminal window, run the client by executing: ./client  
  
# Source Code 
There are two main source code files - server.c and client.c. 
  
**Server.c**
  
The server.c file implements the server-side logic for the Course Registration Portal system that 
supports Admin, Faculty, and Student roles. It uses C socket programming with multithreading 
(pthread) to handle concurrent client connections. The server listens on port 8080 and interacts with 
clients to perform user authentication, course management, and enrollment functions.  
  
Key features include:  
  
● User Authentication: Validates credentials for different user roles (admin, faculty, student).  
  
● Admin Operations: Add/modify/block/activate students and faculty, view user data.  
  
● Faculty Operations: Add, update, remove, and view courses assigned to them.  
  
● Student Operations: View all available courses, enroll/drop courses, and view their enrolled 
courses.  
  
● Data Persistence: Reads from and writes to users.txt, courses.txt, and enrollments.txt using 
file locking to maintain data consistency in concurrent access.  
  
The server uses file-based storage for simplicity and applies read/write locks to prevent race 
conditions.  
  
**Client.c**
  
The client.c file implements the client-side interface of the Academia: Course Registration Portal. It 
connects to the server via TCP sockets and provides role-based menus for three types of users: 
Admin, Faculty, and Student.  
  
Key features:  
  
● Login: Sends the user’s role, username, and password to the server for authentication.  
  
● Admin Interface: Options to add/modify/block/activate students or faculty, and to view user 
records.  
  
● Faculty Interface: Faculty members can add, update, or remove courses they offer, view 
course details, and change passwords.  
  
● Student Interface: Students can view available courses, enroll or drop courses, view their own 
enrolled courses, and update their password.  
  
● User Interaction: Uses standard input/output functions (scanf, printnf) for terminal-based user 
interaction, and socket functions (send, recv) for communication with the server.  
  
The client remains responsive and menu-driven until the user chooses to log out, ensuring a smooth 
and interactive experience for each role.
