#define _POSIX_C_SOURCE 200809L
#include "habit_tracker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

User users[MAX_USERS];
int userCount = 0;
int loggedIn = -1;

/* ------------------------------------------------------------------ styling */

/* Colours only on a real terminal (or FORCE_COLOR), never when NO_COLOR is set. */
static int useColor(void) {
    static int cached = -1;
    if (cached == -1) {
        cached = getenv("NO_COLOR") == NULL && (isatty(STDOUT_FILENO) || getenv("FORCE_COLOR") != NULL);
    }
    return cached;
}

static const char *c(const char *code) { return useColor() ? code : ""; }
#define RESET c("\033[0m")
#define BOLD c("\033[1m")
#define DIM c("\033[2m")
#define GREEN c("\033[32m")
#define BGREEN c("\033[1;32m")
#define YELLOW c("\033[33m")
#define RED c("\033[31m")
#define CYAN c("\033[36m")

static void clearScreen(void) {
    if (useColor()) {
        printf("\033[H\033[2J"); /* ANSI clear: no `clear` process per redraw */
    } else {
        printf("\n");
    }
}

static void displayHeader(const char *title) {
    clearScreen();
    printf("%s╭──────────────────────────────────────╮%s\n", GREEN, RESET);
    printf("%s│%s  %sGoriber Habit Tracker%s               %s│%s\n", GREEN, RESET, BOLD, RESET, GREEN, RESET);
    printf("%s╰──────────────────────────────────────╯%s\n", GREEN, RESET);
    printf("  %s%s%s", BOLD, title, RESET);
    if (loggedIn >= 0) printf("  %s· %s%s", DIM, users[loggedIn].username, RESET);
    printf("\n\n");
}

static void menuItem(int n, const char *label) {
    printf("  %s%d%s  %s\n", BGREEN, n, RESET, label);
}

static void errorMsg(const char *msg) { printf("%s✗ %s%s\n", RED, msg, RESET); }
static void okMsg(const char *msg) { printf("%s✓ %s%s\n", BGREEN, msg, RESET); }

static void formatDuration(int seconds, char *out, size_t size) {
    snprintf(out, size, "%02d:%02d:%02d", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
}

/* ------------------------------------------------------------------ input */

int readLine(const char *prompt, char *buf, size_t size) {
    if (prompt) {
        printf("%s%s›%s ", prompt, CYAN, RESET);
        fflush(stdout);
    }
    if (!fgets(buf, (int)size, stdin)) return 0;
    size_t len = strcspn(buf, "\n");
    if (buf[len] != '\n' && !feof(stdin)) { /* line longer than buf: drop the rest */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
    }
    buf[len] = '\0';
    return 1;
}

int readInt(const char *prompt, int *out) {
    char line[64];
    for (;;) {
        if (!readLine(prompt, line, sizeof line)) return 0;
        char *end;
        long v = strtol(line, &end, 10);
        while (*end == ' ' || *end == '\t') end++;
        if (end != line && *end == '\0') {
            *out = (int)v;
            return 1;
        }
        errorMsg("Please enter a number.");
    }
}

void pressEnterToContinue(void) {
    char line[8];
    printf("\n%sPress Enter to continue…%s", DIM, RESET);
    fflush(stdout);
    readLine(NULL, line, sizeof line); /* exactly one Enter */
}

/* ------------------------------------------------------------------ data */

int findUserIndex(const char *username) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0) return i;
    }
    return -1;
}

int addUser(const char *username) {
    if (userCount >= MAX_USERS || username[0] == '\0' || findUserIndex(username) != -1) return -1;
    User *u = &users[userCount];
    memset(u, 0, sizeof *u);
    snprintf(u->username, sizeof u->username, "%s", username);
    return userCount++;
}

int recordSession(User *u, const char *category, time_t start, time_t end) {
    if (u->sessionCount >= MAX_SESSIONS) return 0;
    StudySession *s = &u->sessions[u->sessionCount++];
    snprintf(s->category, sizeof s->category, "%s", category[0] ? category : "General");
    localtime_r(&start, &s->startTime);
    localtime_r(&end, &s->endTime);
    s->duration = (int)difftime(end, start);
    return 1;
}

/* Days since the epoch in local time (struct tm normalised through mktime). */
static long dayNumber(struct tm t) {
    t.tm_hour = 12;
    t.tm_min = t.tm_sec = 0;
    t.tm_isdst = -1;
    return (long)(mktime(&t) / 86400);
}

int currentStreak(const User *u, time_t now) {
    struct tm today;
    localtime_r(&now, &today);
    long day = dayNumber(today);
    int streak = 0;
    /* A streak survives until the end of the day after the last session. */
    for (int miss = 0;; day--) {
        int studied = 0;
        for (int i = 0; i < u->sessionCount && !studied; i++) {
            studied = dayNumber(u->sessions[i].startTime) == day;
        }
        if (studied) {
            streak++;
        } else if (streak == 0 && miss == 0) {
            miss = 1; /* nothing yet today: still counts if yesterday was studied */
        } else {
            return streak;
        }
    }
}

const char *dataFilePath(void) {
    const char *p = getenv("HABIT_DATA");
    return p && *p ? p : DEFAULT_DATA_FILE;
}

#define DATA_MAGIC 0x48544B31u /* "HTK1" */

int saveData(void) {
    FILE *f = fopen(dataFilePath(), "wb");
    if (!f) return 0;
    unsigned magic = DATA_MAGIC;
    int ok = fwrite(&magic, sizeof magic, 1, f) == 1 && fwrite(&userCount, sizeof userCount, 1, f) == 1 &&
             fwrite(users, sizeof(User), (size_t)userCount, f) == (size_t)userCount;
    return fclose(f) == 0 && ok;
}

int loadData(void) {
    FILE *f = fopen(dataFilePath(), "rb");
    if (!f) return 0;
    unsigned magic = 0;
    int count = 0;
    int ok = fread(&magic, sizeof magic, 1, f) == 1 && magic == DATA_MAGIC && fread(&count, sizeof count, 1, f) == 1 &&
             count >= 0 && count <= MAX_USERS && fread(users, sizeof(User), (size_t)count, f) == (size_t)count;
    fclose(f);
    userCount = ok ? count : 0;
    return ok;
}

/* ------------------------------------------------------------------ menus */

void displayMainMenu(void) {
    displayHeader("Main Menu");
    menuItem(1, "Create new user");
    menuItem(2, "Login");
    menuItem(3, "Exit");
    printf("\n");
}

void displayUserMenu(void) {
    displayHeader("Home");
    int streak = currentStreak(&users[loggedIn], time(NULL));
    if (streak > 0) printf("  %s🔥 %d-day streak%s — keep it going.\n\n", YELLOW, streak, RESET);
    menuItem(1, "Start a study session");
    menuItem(2, "Show statistics");
    menuItem(3, "Manage to-do list");
    menuItem(4, "Logout");
    printf("\n");
}

void createUser(void) {
    char name[NAME_LEN];
    displayHeader("Create New User");
    if (!readLine("Username ", name, sizeof name)) return;
    if (name[0] == '\0') {
        errorMsg("Username cannot be empty.");
    } else if (findUserIndex(name) != -1) {
        errorMsg("That username already exists.");
    } else if (addUser(name) == -1) {
        errorMsg("Maximum number of users reached.");
    } else {
        saveData();
        okMsg("User created. You can log in now.");
    }
    pressEnterToContinue();
}

void loginUser(void) {
    char name[NAME_LEN];
    displayHeader("Login");
    if (!readLine("Username ", name, sizeof name)) return;
    int index = findUserIndex(name);
    if (index == -1) {
        errorMsg("User not found.");
        pressEnterToContinue();
        return;
    }
    loggedIn = index;
}

void startStudyOption(void) {
    if (users[loggedIn].sessionCount >= MAX_SESSIONS) {
        errorMsg("Maximum number of sessions reached.");
        pressEnterToContinue();
        return;
    }
    displayHeader("Study Session");
    menuItem(1, "Stopwatch     — count up until you stop");
    menuItem(2, "Countdown     — fixed length");
    menuItem(3, "Pomodoro      — focus block with pause");
    menuItem(4, "Back");
    printf("\n");
    int choice;
    if (!readInt("Choose ", &choice)) return;
    switch (choice) {
        case 1: startTimer(); break;
        case 2: startCountdown(); break;
        case 3: pomodoroTimer(); break;
        case 4: return;
        default:
            errorMsg("Invalid choice.");
            pressEnterToContinue();
    }
}

static void finishSession(const char *category, time_t start, time_t end) {
    char d[16];
    User *u = &users[loggedIn];
    if (recordSession(u, category, start, end)) {
        saveData();
        formatDuration(u->sessions[u->sessionCount - 1].duration, d, sizeof d);
        printf("\n%s✓ Logged %s of %s.%s\n", BGREEN, d, u->sessions[u->sessionCount - 1].category, RESET);
    }
}

void startTimer(void) {
    char category[NAME_LEN], line[8];
    displayHeader("Stopwatch");
    if (!readLine("What are you studying? ", category, sizeof category)) return;
    time_t start = time(NULL);
    printf("\n  %s● Recording…%s press Enter to stop.\n", RED, RESET);
    readLine(NULL, line, sizeof line);
    finishSession(category, start, time(NULL));
    pressEnterToContinue();
}

void startCountdown(void) {
    char category[NAME_LEN];
    int minutes;
    displayHeader("Countdown");
    if (!readLine("What are you studying? ", category, sizeof category)) return;
    if (!readInt("Minutes ", &minutes)) return;
    if (minutes <= 0 || minutes > 24 * 60) {
        errorMsg("Enter between 1 and 1440 minutes.");
        pressEnterToContinue();
        return;
    }
    time_t start = time(NULL);
    for (int left = minutes * 60; left > 0; left--) {
        char d[16];
        formatDuration(left, d, sizeof d);
        printf("\r  %s⏳ %s%s ", BOLD, d, RESET);
        fflush(stdout);
        sleep(1);
    }
    printf("\r  %s⏰ Time's up!%s     \n", YELLOW, RESET);
    finishSession(category, start, time(NULL));
    pressEnterToContinue();
}

/* Single-key input (no Enter needed) while the pomodoro runs. */
static int rawMode(struct termios *saved) {
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, saved) != 0) return 0;
    struct termios raw = *saved;
    raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    return tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
}

void pomodoroTimer(void) {
    char category[NAME_LEN];
    int minutes;
    displayHeader("Pomodoro");
    if (!readLine("What are you working on? ", category, sizeof category)) return;
    if (!readInt("Focus minutes (25 is classic) ", &minutes)) return;
    if (minutes <= 0 || minutes > 24 * 60) {
        errorMsg("Enter between 1 and 1440 minutes.");
        pressEnterToContinue();
        return;
    }

    struct termios saved;
    int raw = rawMode(&saved);
    int total = minutes * 60, left = total, paused = 0, quit = 0;
    time_t start = time(NULL);
    while (left > 0 && !quit) {
        displayHeader("Pomodoro");
        int width = 30, filled = (total - left) * width / total;
        char d[16];
        formatDuration(left, d, sizeof d);
        printf("  %s%s%s  %s\n\n  %s", BOLD, category[0] ? category : "Focus", RESET, paused ? "⏸ paused" : "▶ focusing", GREEN);
        for (int i = 0; i < width; i++) fputs(i < filled ? "█" : "░", stdout);
        printf("%s  %s\n\n  %sp%s pause/resume   %sq%s quit%s\n", RESET, d, BOLD, RESET, BOLD, RESET, raw ? "" : "   (then Enter)");
        fflush(stdout);

        fd_set set;
        struct timeval timeout = {1, 0};
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0) {
            int ch = getchar();
            if (ch == EOF) quit = 1;
            else if (ch == 'p' || ch == 'P') paused = !paused;
            else if (ch == 'q' || ch == 'Q') quit = 1;
            continue; /* keypress woke us early: don't count it as a second */
        }
        if (!paused) left--;
    }
    if (raw) tcsetattr(STDIN_FILENO, TCSANOW, &saved);

    if (quit) {
        printf("\n%sPomodoro stopped early — not logged.%s\n", YELLOW, RESET);
    } else {
        printf("\n%s🍅 Pomodoro complete!%s\n", BGREEN, RESET);
        time_t end = start + total; /* paused time doesn't count as study */
        finishSession(category, start, end);
    }
    pressEnterToContinue();
}

/* ------------------------------------------------------------------ statistics */

void showStatistics(void) {
    const User *u = &users[loggedIn];
    char d[16];
    displayHeader("Statistics");
    if (u->sessionCount == 0) {
        printf("  No sessions yet. Start one from the home menu.\n");
        pressEnterToContinue();
        return;
    }

    int total = 0;
    for (int i = 0; i < u->sessionCount; i++) total += u->sessions[i].duration;
    formatDuration(total, d, sizeof d);
    printf("  %-16s %s%d%s\n", "Sessions", BOLD, u->sessionCount, RESET);
    printf("  %-16s %s%s%s\n", "Total time", BOLD, d, RESET);
    printf("  %-16s %s%d day%s%s\n\n", "Current streak", YELLOW, currentStreak(u, time(NULL)),
           currentStreak(u, time(NULL)) == 1 ? "" : "s", RESET);

    /* per-category totals with a bar chart */
    char names[MAX_SESSIONS][NAME_LEN];
    int secs[MAX_SESSIONS], n = 0, max = 1;
    for (int i = 0; i < u->sessionCount; i++) {
        int j = 0;
        while (j < n && strcmp(names[j], u->sessions[i].category) != 0) j++;
        if (j == n) {
            snprintf(names[n], NAME_LEN, "%s", u->sessions[i].category);
            secs[n++] = 0;
        }
        secs[j] += u->sessions[i].duration;
        if (secs[j] > max) max = secs[j];
    }
    printf("  %sBy category%s\n", BOLD, RESET);
    for (int j = 0; j < n; j++) {
        int bar = secs[j] * 24 / max;
        formatDuration(secs[j], d, sizeof d);
        printf("  %-12.12s %s", names[j], GREEN);
        for (int k = 0; k < 24; k++) fputs(k < bar ? "█" : " ", stdout);
        printf("%s %s\n", RESET, d);
    }

    printf("\n  %sRecent sessions%s\n", BOLD, RESET);
    for (int i = u->sessionCount - 1, shown = 0; i >= 0 && shown < 5; i--, shown++) {
        char when[32];
        strftime(when, sizeof when, "%a %d %b %H:%M", &u->sessions[i].startTime);
        formatDuration(u->sessions[i].duration, d, sizeof d);
        printf("  %s%s%s  %-12.12s %s\n", DIM, when, RESET, u->sessions[i].category, d);
    }
    pressEnterToContinue();
}

/* ------------------------------------------------------------------ to-do */

void manageToDoList(void) {
    for (;;) {
        User *u = &users[loggedIn];
        displayHeader("To-Do List");
        if (u->todoCount == 0) printf("  %sNothing here yet.%s\n", DIM, RESET);
        for (int i = 0; i < u->todoCount; i++) {
            printf("  %s%2d%s  %s%s%s %s\n", DIM, i + 1, RESET, u->todos[i].completed ? BGREEN : "",
                   u->todos[i].completed ? "[✓]" : "[ ]", RESET, u->todos[i].description);
        }
        printf("\n");
        menuItem(1, "Add items");
        menuItem(2, "Mark an item done");
        menuItem(3, "Back");
        printf("\n");
        int choice;
        if (!readInt("Choose ", &choice)) return;
        if (choice == 1) {
            char text[TEXT_LEN];
            printf("  %sOne per line; empty line to finish.%s\n", DIM, RESET);
            while (u->todoCount < MAX_TODO && readLine("  + ", text, sizeof text) && text[0] != '\0') {
                snprintf(u->todos[u->todoCount].description, TEXT_LEN, "%s", text);
                u->todos[u->todoCount++].completed = 0;
            }
            if (u->todoCount >= MAX_TODO) errorMsg("To-do list is full.");
            saveData();
        } else if (choice == 2) {
            int index;
            if (!readInt("Item number ", &index)) return;
            if (index < 1 || index > u->todoCount) {
                errorMsg("No item with that number.");
                pressEnterToContinue();
            } else {
                u->todos[index - 1].completed = 1;
                saveData();
            }
        } else if (choice == 3) {
            return;
        } else {
            errorMsg("Invalid choice.");
            pressEnterToContinue();
        }
    }
}
