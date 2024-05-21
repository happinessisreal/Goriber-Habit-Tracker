#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <time.h>

#include <unistd.h>



#define MAX_USERS 100

#define MAX_SESSIONS 100

#define MAX_TODO 100



// Define structures for user accounts, study sessions, and to-do items

typedef struct {

    char description[100];

    int completed;

} ToDoItem;



typedef struct {

    char category[50];

    struct tm startTime;

    struct tm endTime;

    int duration; // in seconds

    int sessionActive; // Indicates if a session is currently active

    int countdownActive; // Indicates if a countdown is currently active

    int countdownDuration; // in seconds

} StudySession;



typedef struct {

    char username[50];

    int sessionCount;

    StudySession sessions[MAX_SESSIONS];

    ToDoItem todos[MAX_TODO];

    int todoCount;

} User;



// Global array to store users

User users[MAX_USERS];

int userCount = 0;

int loggedIn = -1; // Index of logged-in user, -1 if no user is logged in



// Function prototypes

void clearScreen();

void displayHeader(const char* header);

void displayMainMenu();

void displayUserMenu();

void createUser();

void loginUser();

void startStudyOption();

void endSession();

void showStatistics();

void manageToDoList();

int findUserIndex(const char* username);

void pressEnterToContinue();

void startTimer();

void startCountdown();

void updateCountdown(int totalSeconds);

void pomodoroTimer();

void pausePomodoroTimer(int *remainingTime);



// Function to clear the screen

void clearScreen() {

    #ifdef _WIN32

        system("cls");

    #else

        system("clear");

    #endif

}



// Function to display a header

void displayHeader(const char* header) {

    clearScreen();

    printf("== %s ==\n\n", header);

}

void displayMainMenu()

 {

   system("color 3F");

    displayHeader("Main Menu");

    printf("1. Create new user\n");

    printf("2. Login\n");

    printf("3. Exit\n");

    printf("Enter your choice: ");

}

// Function to display the main menu





// Function to display the user menu

void displayUserMenu() {

    displayHeader("User Menu");

    printf("1. Start a study session\n");

    printf("2. Show statistics\n");

    printf("3. Manage to-do list\n");

    printf("4. Logout\n");

    printf("Enter your choice: ");

}



// Function to create a new user

void createUser() {

    displayHeader("Create New User");

    if (userCount >= MAX_USERS) {

        printf("Error: Maximum number of users reached.\n");

        pressEnterToContinue();

        return;

    }

    printf("Enter username: ");

    scanf("%49s", users[userCount].username);

    users[userCount].sessionCount = 0;

    users[userCount].todoCount = 0;

    userCount++;

    printf("User created successfully.\n");

    pressEnterToContinue();

}



// Function to login a user

void loginUser() {

    displayHeader("Login");

    printf("Enter username: ");

    char username[50];

    scanf("%49s", username);

    int index = findUserIndex(username);

    if (index == -1) {

        printf("Error: User not found.\n");

        pressEnterToContinue();

    } else {

        loggedIn = index;

        printf("Login successful.\n");

        pressEnterToContinue();

    }

}



// Function to find a user index

int findUserIndex(const char* username) {

    for (int i = 0; i < userCount; i++) {

        if (strcmp(users[i].username, username) == 0) {

            return i;

        }

    }

    return -1;

}



// Function to press enter to continue

void pressEnterToContinue() {

    printf("\nPress Enter to continue...");

    getchar(); // Wait for Enter key

    while (getchar() != '\n'); // Ensure buffer is empty

}



// Function to start a study session option

void startStudyOption() {

    if (loggedIn == -1 || users[loggedIn].sessionCount >= MAX_SESSIONS) {

        printf("Error: No user logged in or maximum sessions reached.\n");

        pressEnterToContinue();

        return;

    }

    displayHeader("Study Session Options");

    printf("1. Timer\n");

    printf("2. Countdown\n");

    printf("3. Pomodoro Timer\n");

    printf("4. Exit to User Menu\n");

    printf("Enter your choice: ");

    int choice;

    scanf("%d", &choice);

    getchar(); // Clear the newline character after the number input



    switch (choice) {

        case 1:

            startTimer();

            break;

        case 2:

            startCountdown();

            break;

        case 3:

            pomodoroTimer();

            break;

        case 4:

            return; // Exit to user menu

        default:

            printf("Invalid choice. Please try again.\n");

            pressEnterToContinue();

    }

}



// Function to start a timer

void startTimer() {

    if (users[loggedIn].sessions[users[loggedIn].sessionCount].sessionActive) {

        printf("Error: A session is already active. Please end the current session before starting a new one.\n");

        pressEnterToContinue();

        return;

    }

    displayHeader("Starting a New Study Session");

    printf("Enter study category: ");

    scanf("%49s", users[loggedIn].sessions[users[loggedIn].sessionCount].category);

    getchar(); // Clear the newline character after the input

    users[loggedIn].sessions[users[loggedIn].sessionCount].sessionActive = 1;

    time_t start = time(NULL);

    users[loggedIn].sessions[users[loggedIn].sessionCount].startTime = *localtime(&start);

    displayHeader("Timer is on");

    printf("Press 'Enter' to stop the timer...\n");

    while (getchar() != '\n'); // Wait for Enter key

    time_t end = time(NULL);

    users[loggedIn].sessions[users[loggedIn].sessionCount].endTime = *localtime(&end);

    users[loggedIn].sessions[users[loggedIn].sessionCount].duration = (int)difftime(end, start);

    users[loggedIn].sessions[users[loggedIn].sessionCount].sessionActive = 0;

    users[loggedIn].sessionCount++;

    printf("Study session ended. Duration: %02d:%02d:%02d.\n",

           users[loggedIn].sessions[users[loggedIn].sessionCount - 1].duration / 3600,

           (users[loggedIn].sessions[users[loggedIn].sessionCount - 1].duration % 3600) / 60,

           users[loggedIn].sessions[users[loggedIn].sessionCount - 1].duration % 60);

    pressEnterToContinue();

}



// Function to start a countdown

void startCountdown() {

    if (users[loggedIn].sessions[users[loggedIn].sessionCount].countdownActive) {

        printf("Error: A countdown is already active. Please wait for it to finish before starting a new one.\n");

        pressEnterToContinue();

        return;

    }

    displayHeader("Starting a New Countdown");

    printf("Enter countdown time in HH MM SS format: ");

    int hours, minutes, seconds;

    scanf("%d %d %d", &hours, &minutes, &seconds);

    getchar(); // Clear the newline character after the number input

    int totalSeconds = hours * 3600 + minutes * 60 + seconds;

    if (totalSeconds <= 0 || hours < 0 || minutes < 0 || minutes >= 60 || seconds < 0 || seconds >= 60) {

        printf("Error: Invalid countdown time.\n");

        pressEnterToContinue();

        return;

    }

    users[loggedIn].sessions[users[loggedIn].sessionCount].countdownDuration = totalSeconds;

    users[loggedIn].sessions[users[loggedIn].sessionCount].countdownActive = 1;

    updateCountdown(totalSeconds);

}



// Function to update countdown

void updateCountdown(int totalSeconds) {

    displayHeader("Countdown");

    while (totalSeconds > 0) {

        int hours = totalSeconds / 3600;

        int minutes = (totalSeconds % 3600) / 60;

        int seconds = totalSeconds % 60;

        printf("\r%02d:%02d:%02d", hours, minutes, seconds);

        fflush(stdout);

        sleep(1);

        totalSeconds--;

    }

    printf("\nCountdown finished.\n");

    users[loggedIn].sessions[users[loggedIn].sessionCount].countdownActive = 0;

    pressEnterToContinue();

}



// Function to show statistics

void showStatistics() {

    if (loggedIn == -1) {

        printf("Error: No user logged in.\n");

        pressEnterToContinue();

        return;

    }

    displayHeader("Statistics");

    printf("Total study sessions: %d\n", users[loggedIn].sessionCount);

    for (int i = 0; i < users[loggedIn].sessionCount; i++) {

        printf("Session %d - Category: %s, Duration: %02d:%02d:%02d\n",

               i + 1,

               users[loggedIn].sessions[i].category,

               users[loggedIn].sessions[i].duration / 3600,

               (users[loggedIn].sessions[i].duration % 3600) / 60,

               users[loggedIn].sessions[i].duration % 60);

    }

    pressEnterToContinue();

}



// Function to manage the to-do list

void manageToDoList() {

    int choice;

    do {

        if (loggedIn == -1) {

            printf("Error: No user logged in.\n");

            pressEnterToContinue();

            return;

        }

        displayHeader("Manage To-Do List");

        printf("1. Add a to-do item\n");

        printf("2. Mark a to-do item as completed\n");

        printf("3. Show to-do list\n");

        printf("4. Return to User Menu\n");

        printf("Enter your choice: ");

        scanf("%d", &choice);

        getchar(); // Clear the newline character after the number input



        switch (choice) {

            case 1: // Add a to-do item

                do {

                    if (users[loggedIn].todoCount >= MAX_TODO) {

                        printf("Error: To-do list is full.\n");

                        break;

                    }

                    displayHeader("Add To-Do Item");

                    printf("Enter the description of the to-do item (or type 'exit' to finish adding): ");

                    char input[100];

                    scanf("%99[^\n]", input);

                    getchar(); // Clear the newline character after the input

                    if (strcmp(input, "exit") == 0) {

                        break;

                    }

                    strcpy(users[loggedIn].todos[users[loggedIn].todoCount].description, input);

                    users[loggedIn].todos[users[loggedIn].todoCount].completed = 0;

                    users[loggedIn].todoCount++;

                    printf("To-do item added. You can add another item or type 'exit' to finish.\n");

                } while (users[loggedIn].todoCount < MAX_TODO);

                break;

            case 2: // Mark a to-do item as completed

                displayHeader("Mark To-Do Item as Completed");

                printf("Enter the index of the to-do item to mark as completed: ");

                int index;

                scanf("%d", &index);

                getchar(); // Clear the newline character after the input

                if (index < 1 || index > users[loggedIn].todoCount) {

                    printf("Error: Invalid to-do item index.\n");

                } else {

                    users[loggedIn].todos[index - 1].completed = 1;

                    printf("To-do item marked as completed.\n");

                }

                break;

            case 3: // Show to-do list

                displayHeader("To-Do List");

                for (int i = 0; i < users[loggedIn].todoCount; i++) {

                    printf("%d. %s [%s]\n", i + 1, users[loggedIn].todos[i].description,

                           users[loggedIn].todos[i].completed ? "Completed" : "Pending");

                }

                break;

            case 4: // Return to User Menu

                return;

        }

        pressEnterToContinue();

    } while (choice != 4);

}



// Function to handle Pomodoro Timer

void pomodoroTimer() {

    displayHeader("Pomodoro Timer");

    printf("Enter Pomodoro session duration in minutes: ");

    int duration;

    scanf("%d", &duration);

    getchar(); // Clear the newline character after the input



    if (duration <= 0) {

        printf("Error: Invalid duration.\n");

        pressEnterToContinue();

        return;

    }



    int totalSeconds = duration * 60;

    int paused = 0;

    int remainingTime = totalSeconds;



    while (remainingTime > 0) {

        displayHeader("Pomodoro Timer");

        printf("Press 'p' to pause/resume the timer.\n");

        printf("Press 'q' to quit the timer.\n");

        printf("Time remaining: %02d:%02d\n", remainingTime / 60, remainingTime % 60);



        fd_set set;

        struct timeval timeout;

        FD_ZERO(&set);

        FD_SET(STDIN_FILENO, &set);

        timeout.tv_sec = 1;

        timeout.tv_usec = 0;



        int res = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);

        if (res > 0) {

            char c = getchar();

            if (c == 'p') {

                paused = !paused;

            } else if (c == 'q') {

                printf("Pomodoro timer quit.\n");

                pressEnterToContinue();

                return;

            }

        }



        if (!paused) {

            remainingTime--;

        }

    }



    printf("Pomodoro session completed!\n");

    pressEnterToContinue();

}


