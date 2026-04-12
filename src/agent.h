#ifndef AGENT_H
#define AGENT_H

#include <stddef.h>
#include <stdint.h>
#include "holo.h"

#define MAX_MAILBOX 50

typedef struct Room Room;

typedef struct MailboxMessage {
    char from[HOLO_MAX_NAME];
    char text[HOLO_MAX_INPUT];
    int read;
    struct MailboxMessage *next;
} MailboxMessage;

typedef struct Equipment {
    int id;
    char name[HOLO_MAX_NAME];
    int granted;
    struct Equipment *next;
} Equipment;

typedef enum {
    AGENT_STATE_DISCONNECTED,
    AGENT_STATE_CONNECTED,
    AGENT_STATE_IN_ROOM
} AgentState;

typedef struct Agent {
    int fd;
    char name[HOLO_MAX_NAME];
    AgentState state;
    struct Room *room;
    int permission_level;
    char input_buffer[HOLO_MAX_INPUT];
    size_t input_pos;
    char output_buffer[HOLO_MAX_OUTPUT];
    size_t output_pos;
    MailboxMessage *mailbox;
    int mailbox_count;
    Equipment *equipment;
    int equipment_count;
    struct Agent *next;
} Agent;

Agent *agent_create(int fd);
void agent_destroy(Agent *agent);
void agent_set_name(Agent *agent, const char *name);
void agent_set_room(Agent *agent, struct Room *room);
struct Room *agent_get_room(const Agent *agent);
void agent_set_permission(Agent *agent, int level);
int agent_get_permission(const Agent *agent);
int agent_read_input(Agent *agent);
int agent_write_output(Agent *agent);
void agent_send(Agent *agent, const char *message);
void agent_mailbox_send(Agent *agent, const char *from, const char *text);
int agent_mailbox_count(const Agent *agent);
const MailboxMessage *agent_mailbox_get(const Agent *agent);
void agent_mailbox_clear(Agent *agent);
void agent_equipment_grant(Agent *agent, int id, const char *name);
int agent_equipment_has(const Agent *agent, int id);

#endif
