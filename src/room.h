#ifndef ROOM_H
#define ROOM_H

#include "holodeck.h"

typedef struct Exit {
    char direction[HOLO_MAX_NAME];
    Room *destination;
    struct Exit *next;
} Exit;

typedef struct Note {
    char author[HOLO_MAX_NAME];
    char text[HOLO_MAX_DESC];
    struct Note *next;
} Note;

struct Room {
    char id[HOLO_MAX_NAME];
    char name[HOLO_MAX_NAME];
    char description[HOLO_MAX_DESC];
    Exit *exits;
    Note *notes;
    Agent *agents[HOLO_MAX_AGENTS];
    int agent_count;
    int booted;
    void *runtime_data;
};

Room *room_create(const char *id, const char *name, const char *description);
void room_destroy(Room *room);
void room_connect(Room *from, Room *to, const char *direction);
void room_disconnect(Room *room, const char *direction);
Room *room_find_exit(const Room *room, const char *direction);
void room_add_agent(Room *room, Agent *agent);
void room_remove_agent(Room *room, Agent *agent);
void room_add_note(Room *room, const char *author, const char *text);
const Note *room_get_notes(const Room *room);
int room_is_booted(const Room *room);
void room_set_booted(Room *room, int booted);

#endif
