#include <stdio.h>

#include "habit_tracker.h"

int main(void) {
    loadData();
    int choice;
    for (;;) {
        if (loggedIn == -1) {
            displayMainMenu();
            if (!readInt("Choose ", &choice)) break; /* EOF */
            switch (choice) {
                case 1: createUser(); break;
                case 2: loginUser(); break;
                case 3: saveData(); printf("Goodbye — see you tomorrow.\n"); return 0;
                default: printf("Invalid choice.\n"); pressEnterToContinue();
            }
        } else {
            displayUserMenu();
            if (!readInt("Choose ", &choice)) break;
            switch (choice) {
                case 1: startStudyOption(); break;
                case 2: showStatistics(); break;
                case 3: manageToDoList(); break;
                case 4: loggedIn = -1; break;
                default: printf("Invalid choice.\n"); pressEnterToContinue();
            }
        }
    }
    saveData();
    printf("\n");
    return 0;
}
