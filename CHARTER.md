# Holodeck C — Charter

## Mission
Implement the Holodeck Studio core protocol in pure C99.

## What to Build
A minimal MUD server that implements the 40-point Holodeck Conformance Suite:
- Socket server (select or epoll)
- Room graph (create, destroy, connect, describe)
- Agent sessions (enter, leave, command parsing)
- Communication channels (say, tell, yell, gossip, note)
- Live connections (HTTP client, shell exec)
- Room-as-runtime (boot on enter, shutdown on leave)
- Combat/oversight tick cycle
- Living manual (read/write/evolve)

## Architecture
```
src/
  main.c          — event loop, socket accept
  room.c/h        — room graph, exits, descriptions
  agent.c/h       — agent session, command dispatch
  command.c/h     — command parser and handlers
  comms.c/h       — say/tell/yell/gossip/note/mailbox
  connection.c/h  — live connections (HTTP, shell)
  runtime.c/h     — room boot/shutdown, gauges
  combat.c/h      — oversight ticks, evolving scripts
  manual.c/h      — living manual, generations
  conformance.c/h — test harness for 40 conformance tests
```

## Constraints
- C99 only, no external dependencies beyond POSIX
- Must compile with `gcc -Wall -Wextra -pedantic -std=c99`
- Memory-managed: no leaks, valgrind-clean
- Each module has a corresponding test file
- Must pass 40/40 conformance tests

## The Deep Question
What IS a room at the byte level? A struct with exits as pointers? A linked graph?
What IS a command at the byte level? A string comparison? A function pointer dispatch?
What IS an agent session? A file descriptor with a buffer?
Build this and document what you learn.
