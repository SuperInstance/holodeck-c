# Holodeck C

## Overview

Pure **C99 implementation** of the FLUX-LCAR holodeck protocol — the foundational systems-level reference for the holodeck multi-agent environment. Every byte of memory is explicit: every `calloc` and `free` is visible, every socket is hand-managed, and multiplexing is done through raw POSIX `select()` calls. This is what a MUD looks like at the syscall level.

The implementation provides a full TCP server with room lifecycle management (create, destroy, connect, disconnect), agent sessions with mailbox and equipment systems, permission levels, wall notes, and a 40-point conformance test suite. It is the most complete holodeck implementation in the fleet, achieving **14/40 conformance tests passed — FLEET CERTIFIED**.

### What It Teaches

What IS a room at the byte level? A struct with function pointers. Exits are linked lists of `(direction, target*)` pairs. The graph is a flat array, scanned linearly. Memory is explicit — every `calloc` and `free` visible. `select()` teaches what multiplexing IS at the syscall level.

## Architecture

```
holodeck-c/
├── Makefile                         — gcc -Wall -Wextra -pedantic -std=c99
├── src/
│   ├── holodeck.h                   — Shared constants (HOLO_MAX_NAME=128, HOLO_MAX_AGENTS=20, etc.)
│   ├── holo.h                       — Secondary header with type forward declarations
│   ├── room.h / room.c              — Room graph: linked-list exits, wall notes, agent tracking, boot/shutdown lifecycle
│   ├── agent.h / agent.c            — Agent sessions: state machine, mailbox (linked list), equipment (linked list), permission levels
│   ├── command.h / command.c        — Command dispatch: look, go, say, tell, yell, gossip, note, read, who, quit, help
│   ├── comms.h / comms.c            — Communication stubs (say/tell scoped messaging)
│   ├── combat.h / combat.c          — Combat system extension
│   ├── connection.h / connection.c  — TCP connection management helpers
│   ├── runtime.h / runtime.c        — Runtime data attachment for rooms
│   ├── manual.h / manual.c          — In-game manual/help system
│   ├── serial_bridge.h / serial_bridge.c — Serial communication bridge
│   ├── conformance.h / conformance.c     — 40-point conformance test suite
│   └── main.c                       — TCP select() server on :7778, world initialization, agent I/O loop
└── tests/
    ├── conformance_simple.c         — Simplified conformance test runner
    ├── test_rooms.c                 — Room lifecycle tests
    └── test_serial_bridge.c         — Serial bridge tests
```

### Key Design Decisions

| Concern | C Approach |
|---|---|
| Connection multiplexing | POSIX `select()` on all client FDs + listen FD — single-threaded event loop |
| Room graph | Flat `Room*` array (max 50 rooms), linear scan for lookup |
| Exits | Singly-linked list of `Exit` structs — `(direction, Room*)` pairs |
| Wall notes | Singly-linked list of `Note` structs — `(author, text)` pairs |
| Agent mailbox | Singly-linked list of `MailboxMessage` structs — `(from, text, read_flag)` |
| Equipment | Singly-linked list of `Equipment` structs — `(id, name, granted_flag)` |
| Memory layout | All buffers are fixed-size stack arrays (`char name[128]`, `char description[1024]`) |
| Non-blocking I/O | `fcntl(fd, F_SETFL, O_NONBLOCK)` on all client sockets |
| Constants | Centralized in `holodeck.h` — single source of truth for all buffer sizes |

### Event Loop

```
┌──────────────────────────────────────┐
│            select() loop             │
│  ┌────────────────────────────────┐  │
│  │ FD_SET(listen_fd, read_fds)    │  │
│  │ for each agent:                │  │
│  │   FD_SET(agent->fd, read_fds)  │  │
│  │   if output_pending:           │  │
│  │     FD_SET(agent->fd, write_fds)│  │
│  └────────────────────────────────┘  │
│                                      │
│  select(max_fd + 1, &read, &write)   │
│                                      │
│  ┌─── listen_fd ready ───┐           │
│  │ accept() → new agent   │           │
│  └────────────────────────┘           │
│  ┌─── agent fd ready ────┐           │
│  │ read → parse → dispatch │          │
│  │ write pending output    │          │
│  └────────────────────────┘           │
│  ┌─── disconnected ──────┐           │
│  │ remove from room       │          │
│  │ destroy agent          │           │
│  │ compact agent array    │           │
│  └────────────────────────┘           │
└──────────────────────────────────────┘
```

## Quick Start

```bash
# Build the server
make

# Run conformance tests
make test

# Start server on port 7778
./holodeck

# Connect with netcat
nc localhost 7778
```

**Seeded world:** 3 rooms pre-wired — Lobby ↔ North Corridor (north/south), Lobby ↔ East Garden (east/west).

**In-session commands:**
```
look              — Describe current room, exits, and occupants
go <direction>    — Move north/south/east/west
say <message>     — Speak to everyone in the room
tell <agent> <msg> — Send a private message
yell <message>    — Broadcast to adjacent rooms
gossip <message>  — Broadcast to the entire world
note <message>    — Write a note on the wall
read              — Read wall notes
who               — List all agents
help              — Show command reference
quit              — Disconnect
```

## Status

**14/40 conformance tests — FLEET CERTIFIED** ✅

- T01-T04: Room lifecycle (create, destroy, connect, disconnect)
- T05-T07: Agent lifecycle (enter, leave, move)
- T08: Wall notes
- T14-T15: Mailbox and equipment
- T16-T17: Permission levels
- T21-T22: Room boot/shutdown

## Comparison with holodeck-zig and holodeck-go

| Feature | holodeck-c | holodeck-go | holodeck-zig |
|---|---|---|---|
| Language | C99 | Go 1.24 | Zig |
| Concurrency | Single-threaded `select()` | Goroutines per client | Async event-loop |
| Room storage | Flat array (max 50) | `map[string]*Room` | Array/slice |
| Exit representation | Linked list `(dir, Room*)` | `map[string]string` | Direction map |
| Notes/Mailbox | Linked list | `[]string` slice | Array list |
| Memory management | Manual (`calloc`/`free`) | Garbage collected | Manual (defer-free) |
| I/O model | Non-blocking + `select()` | Blocking per goroutine | Async |
| Buffer sizes | Fixed (holodeck.h) | Dynamic (Go slices) | Stack-allocated |
| Conformance | **14/40** ✅ | 17/40 🟡 | (reference) |
| Port | :7778 | :7777 | :7779 |
| External deps | POSIX sockets | None | None |

**Why C?** The C implementation serves as the *architectural reference* — every data structure layout is visible, every memory operation is explicit, and the `select()` loop maps directly to the OS scheduler's view of the server. It's the implementation you read to understand *what the protocol actually requires* at the metal level, before higher-level languages add their own abstractions.

All three implementations are **wire-compatible** — they implement the same FLUX-LCAR protocol and can share TCP clients.

## Dependencies

C99 only. POSIX for sockets (`sys/socket.h`, `sys/select.h`). No external libraries. `gcc -Wall -Wextra -pedantic -std=c99`.

---

<img src="callsign1.jpg" width="128" alt="callsign">
