#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <time.h>

#include <unistd.h>

#include "Function.h"

// Main function

int main() {

    int choice;

    while (1) {

        if (loggedIn == -1) {

            displayMainMenu();

            scanf("%d", &choice);

            getchar(); // Clear the newline character after the number input

            switch (choice) {

                case 1:

                    createUser();

                    break;

                case 2:

                    loginUser();

                    break;

                case 3:

                    printf("Exiting program.\n");

                    return 0;

                default:

                    printf("Invalid choice. Please try again.\n");

                    pressEnterToContinue();

            }

        } else {

            displayUserMenu();

            scanf("%d", &choice);

            getchar(); // Clear the newline character after the number input

            switch (choice) {

                case 1:

                    startStudyOption();

                    break;

                case 2:

                    showStatistics();

                    break;

                case 3:

                    manageToDoList();

                    break;

                case 4:

                    loggedIn = -1;

                    printf("Logged out successfully.\n");

                    pressEnterToContinue();

                    break;

                default:

                    printf("Invalid choice. Please try again.\n");

                    pressEnterToContinue();

            }

        }

    }

    return 0;

}

