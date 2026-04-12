#include "room.h"
#include "agent.h"
#include <stdlib.h>
#include <string.h>

Room *room_create(const char *id, const char *name, const char *description) {
    Room *room = calloc(1, sizeof(Room));
    if (!room) return NULL;

    strncpy(room->id, id, HOLO_MAX_NAME - 1);
    strncpy(room->name, name, HOLO_MAX_NAME - 1);
    strncpy(room->description, description, HOLO_MAX_DESC - 1);
    room->exits = NULL;
    room->notes = NULL;
    room->agent_count = 0;
    room->booted = 0;
    room->runtime_data = NULL;

    for (int i = 0; i < HOLO_MAX_AGENTS; i++) {
        room->agents[i] = NULL;
    }

    return room;
}

void room_destroy(Room *room) {
    if (!room) return;

    Exit *exit = room->exits;
    while (exit) {
        Exit *next = exit->next;
        free(exit);
        exit = next;
    }

    Note *note = room->notes;
    while (note) {
        Note *next = note->next;
        free(note);
        note = next;
    }

    free(room);
}

void room_connect(Room *from, Room *to, const char *direction) {
    if (!from || !to || !direction) return;

    Exit *exit = calloc(1, sizeof(Exit));
    if (!exit) return;

    strncpy(exit->direction, direction, HOLO_MAX_NAME - 1);
    exit->destination = to;
    exit->next = from->exits;
    from->exits = exit;
}

void room_disconnect(Room *room, const char *direction) {
    if (!room || !direction) return;

    Exit **prev = &room->exits;
    Exit *exit = room->exits;

    while (exit) {
        if (strcmp(exit->direction, direction) == 0) {
            *prev = exit->next;
            free(exit);
            return;
        }
        prev = &exit->next;
        exit = exit->next;
    }
}

Room *room_find_exit(const Room *room, const char *direction) {
    if (!room || !direction) return NULL;

    Exit *exit = room->exits;
    while (exit) {
        if (strcmp(exit->direction, direction) == 0) {
            return exit->destination;
        }
        exit = exit->next;
    }

    return NULL;
}

void room_add_agent(Room *room, Agent *agent) {
    if (!room || !agent) return;
    if (room->agent_count >= HOLO_MAX_AGENTS) return;

    for (int i = 0; i < HOLO_MAX_AGENTS; i++) {
        if (room->agents[i] == NULL) {
            room->agents[i] = agent;
            room->agent_count++;
            break;
        }
    }
}

void room_remove_agent(Room *room, Agent *agent) {
    if (!room || !agent) return;

    for (int i = 0; i < HOLO_MAX_AGENTS; i++) {
        if (room->agents[i] == agent) {
            room->agents[i] = NULL;
            room->agent_count--;
            break;
        }
    }
}

int room_agent_count(const Room *room) {
    return room ? room->agent_count : 0;
}

void room_add_note(Room *room, const char *author, const char *text) {
    if (!room || !author || !text) return;

    Note *note = calloc(1, sizeof(Note));
    if (!note) return;

    strncpy(note->author, author, HOLO_MAX_NAME - 1);
    strncpy(note->text, text, HOLO_MAX_DESC - 1);
    note->next = room->notes;
    room->notes = note;
}

const Note *room_get_notes(const Room *room) {
    return room ? room->notes : NULL;
}

int room_is_booted(const Room *room) {
    return room ? room->booted : 0;
}

void room_set_booted(Room *room, int booted) {
    if (room) room->booted = booted;
}
