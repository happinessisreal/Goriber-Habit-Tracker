/* Dev tool: writes a realistic demo data file (used for the README screenshots). */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "habit_tracker.h"

int main(void) {
    time_t now = time(NULL);
    User *u = &users[addUser("rafi")];
    struct { int daysAgo, hour, minutes; const char *cat; } s[] = {
        {6, 20, 50, "Physics"}, {4, 9, 25, "Math"}, {4, 21, 40, "Programming"}, {3, 10, 25, "Math"},
        {3, 16, 55, "Bangla"}, {2, 11, 25, "Math"}, {2, 19, 70, "Programming"}, {1, 8, 25, "Physics"},
        {1, 14, 25, "Math"}, {1, 22, 45, "Programming"}, {0, 9, 25, "Math"}, {0, 11, 35, "Physics"},
    };
    for (size_t i = 0; i < sizeof s / sizeof *s; i++) {
        struct tm t;
        time_t day = now - (time_t)s[i].daysAgo * 86400;
        localtime_r(&day, &t);
        t.tm_hour = s[i].hour;
        t.tm_min = 5;
        t.tm_sec = 0;
        time_t start = mktime(&t);
        recordSession(u, s[i].cat, start, start + s[i].minutes * 60);
    }
    const char *todos[] = {"Finish calculus worksheet", "Revise Newton's laws", "Push the C project to GitHub", "Read 20 pages"};
    for (int i = 0; i < 4; i++) {
        snprintf(u->todos[i].description, TEXT_LEN, "%s", todos[i]);
        u->todos[i].completed = i < 2;
    }
    u->todoCount = 4;
    return saveData() ? 0 : 1;
}
