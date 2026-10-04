#ifndef HABIT_TRACKER_H
#define HABIT_TRACKER_H

#include <stddef.h>
#include <time.h>

#define MAX_USERS 100
#define MAX_SESSIONS 100
#define MAX_TODO 100
#define NAME_LEN 50
#define TEXT_LEN 100
#define DEFAULT_DATA_FILE "habit_data.dat"

typedef struct {
    char description[TEXT_LEN];
    int completed;
} ToDoItem;

typedef struct {
    char category[NAME_LEN];
    struct tm startTime;
    struct tm endTime;
    int duration; /* seconds */
} StudySession;

typedef struct {
    char username[NAME_LEN];
    int sessionCount;
    StudySession sessions[MAX_SESSIONS];
    ToDoItem todos[MAX_TODO];
    int todoCount;
} User;

extern User users[MAX_USERS];
extern int userCount;
extern int loggedIn; /* index into users, -1 when nobody is logged in */

/* ---- input (line based, so bad input never loops forever) ---- */
int readLine(const char *prompt, char *buf, size_t size); /* 0 on EOF */
int readInt(const char *prompt, int *out);                /* 0 on EOF; re-prompts on junk */
void pressEnterToContinue(void);

/* ---- screens ---- */
void displayMainMenu(void);
void displayUserMenu(void);
void createUser(void);
void loginUser(void);
void startStudyOption(void);
void startTimer(void);
void startCountdown(void);
void pomodoroTimer(void);
void showStatistics(void);
void manageToDoList(void);

/* ---- data ---- */
int findUserIndex(const char *username);
int addUser(const char *username);                                    /* index, or -1 */
int recordSession(User *u, const char *category, time_t start, time_t end); /* 0 if full */
int currentStreak(const User *u, time_t now);                         /* consecutive days with a session */
const char *dataFilePath(void);                                       /* $HABIT_DATA or habit_data.dat */
int saveData(void);
int loadData(void);

#endif
