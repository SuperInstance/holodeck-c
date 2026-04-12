CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -Isrc -g -Wno-pedantic
CORE_SRC = src/room.c src/agent.c src/command.c src/comms.c
ALL_SRC = $(CORE_SRC) src/combat.c src/connection.c src/manual.c src/runtime.c
BIN = holodeck

all: $(BIN) test

$(BIN): src/main.c $(ALL_SRC) src/holodeck.h
	$(CC) $(CFLAGS) -o $@ src/main.c $(ALL_SRC)

test: tests/conformance_simple.c $(CORE_SRC) src/holodeck.h
	$(CC) $(CFLAGS) -o test_conf tests/conformance_simple.c $(CORE_SRC)
	./test_conf

clean:
	rm -f $(BIN) test_conf *.o src/*.o

.PHONY: all test clean
