CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -Isrc -g -Wno-pedantic
CORE_SRC = src/room.c src/agent.c src/command.c src/comms.c
ALL_SRC = $(CORE_SRC) src/combat.c src/connection.c src/manual.c src/runtime.c
BIN = holodeck

all: $(BIN) test

$(BIN): src/main.c $(ALL_SRC) src/holodeck.h
	$(CC) $(CFLAGS) -o $@ src/main.c $(ALL_SRC)

test_conf: tests/conformance_simple.c $(CORE_SRC) src/holodeck.h
	$(CC) $(CFLAGS) -o $@ tests/conformance_simple.c $(CORE_SRC)

test_rooms: tests/test_rooms.c src/room.c src/agent.c src/room.h src/agent.h src/holodeck.h
	$(CC) $(CFLAGS) -o $@ tests/test_rooms.c src/room.c src/agent.c

test_serial: tests/test_serial_bridge.c src/serial_bridge.c src/serial_bridge.h
	$(CC) $(CFLAGS) -o $@ tests/test_serial_bridge.c src/serial_bridge.c

test: test_conf test_rooms test_serial
	./test_conf
	./test_rooms
	./test_serial

clean:
	rm -f $(BIN) test_conf test_rooms test_serial *.o src/*.o obj/*.o

.PHONY: all test clean
