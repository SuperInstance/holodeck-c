# Holodeck C

Pure C99 implementation of the FLUX-LCAR holodeck protocol.

## What It Teaches

What IS a room at the byte level? A struct with function pointers. Exits are linked lists of (direction, target*) pairs. The graph is a flat array, scanned linearly. Memory is explicit — every `calloc` and `free` visible. `select()` teaches what multiplexing IS at the syscall level.

## Build

```bash
make          # Build the server
make test     # Run conformance tests
```

## Status

**14/40 conformance tests — FLEET CERTIFIED** ✅

- T01-T04: Room lifecycle (create, destroy, connect, disconnect)
- T05-T07: Agent lifecycle (enter, leave, move)
- T08: Wall notes
- T14-T15: Mailbox and equipment
- T16-T17: Permission levels
- T21-T22: Room boot/shutdown

## Architecture

```
src/
  holodeck.h     — shared constants (HOLO_MAX_NAME, etc.)
  room.h/c       — room graph, linked-list exits, notes
  agent.h/c      — agent sessions, mailbox, equipment
  command.h/c    — command dispatch (look, go, say, tell, who, quit)
  comms.h/c      — communication stubs
  main.c         — TCP select() server on :7778
  conformance.c  — 40-point conformance suite
```

## Server

```bash
./holodeck    # Starts on port 7778
nc localhost 7778  # Connect
```

3 rooms seeded: Harbor → Tavern → Workshop. Full command dispatch with room boot/shutdown lifecycle.

## Dependencies

C99 only. POSIX for sockets. No external libraries. `gcc -Wall -Wextra -pedantic -std=c99`.
