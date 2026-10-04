CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wformat=2
CPPFLAGS += -Iinclude
SRC      = src/main.c src/habit_tracker.c
BIN      = habit-tracker

.PHONY: all run test clean

all: $(BIN)

$(BIN): $(SRC) include/habit_tracker.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $@

run: $(BIN)
	./$(BIN)

test: $(BIN)
	./tests/smoke.sh ./$(BIN)

clean:
	rm -f $(BIN) tests/demo_data
