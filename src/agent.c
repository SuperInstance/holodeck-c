#include "agent.h"
#include "room.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Agent *agent_create(int fd) {
    Agent *agent = calloc(1, sizeof(Agent));
    if (!agent) return NULL;

    agent->fd = fd;
    agent->name[0] = '\0';
    agent->state = AGENT_STATE_CONNECTED;
    agent->room = NULL;
    agent->permission_level = 0;
    agent->input_pos = 0;
    agent->output_pos = 0;
    agent->mailbox = NULL;
    agent->mailbox_count = 0;
    agent->equipment = NULL;
    agent->equipment_count = 0;
    agent->next = NULL;

    return agent;
}

void agent_destroy(Agent *agent) {
    if (!agent) return;

    MailboxMessage *msg = agent->mailbox;
    while (msg) {
        MailboxMessage *next = msg->next;
        free(msg);
        msg = next;
    }

    Equipment *eq = agent->equipment;
    while (eq) {
        Equipment *next = eq->next;
        free(eq);
        eq = next;
    }

    if (agent->fd >= 0) {
        close(agent->fd);
    }

    free(agent);
}

void agent_set_name(Agent *agent, const char *name) {
    if (agent && name) {
        strncpy(agent->name, name, HOLO_MAX_NAME - 1);
    }
}

void agent_set_room(Agent *agent, struct Room *room) {
    if (agent) {
        agent->room = room;
        if (room) {
            agent->state = AGENT_STATE_IN_ROOM;
        } else {
            agent->state = AGENT_STATE_CONNECTED;
        }
    }
}

struct Room *agent_get_room(const Agent *agent) {
    return agent ? agent->room : NULL;
}

void agent_set_permission(Agent *agent, int level) {
    if (agent) {
        agent->permission_level = level;
    }
}

int agent_get_permission(const Agent *agent) {
    return agent ? agent->permission_level : 0;
}

int agent_read_input(Agent *agent) {
    if (!agent || agent->fd < 0) return -1;

    size_t remaining = HOLO_MAX_INPUT - agent->input_pos;
    if (remaining == 0) return 0;

    ssize_t n = read(agent->fd, agent->input_buffer + agent->input_pos, remaining);
    if (n <= 0) return -1;

    agent->input_pos += n;
    return n;
}

int agent_write_output(Agent *agent) {
    if (!agent || agent->fd < 0) return -1;
    if (agent->output_pos == 0) return 0;

    ssize_t n = write(agent->fd, agent->output_buffer, agent->output_pos);
    if (n < 0) return -1;

    if (n < (ssize_t)agent->output_pos) {
        memmove(agent->output_buffer, agent->output_buffer + n, agent->output_pos - n);
    }
    agent->output_pos -= n;

    return n;
}

void agent_send(Agent *agent, const char *message) {
    if (!agent || !message) return;

    size_t msg_len = strlen(message);
    size_t remaining = HOLO_MAX_OUTPUT - agent->output_pos;

    if (msg_len >= remaining) {
        msg_len = remaining - 1;
    }

    memcpy(agent->output_buffer + agent->output_pos, message, msg_len);
    agent->output_pos += msg_len;
}

void agent_mailbox_send(Agent *agent, const char *from, const char *text) {
    if (!agent || !from || !text) return;
    if (agent->mailbox_count >= MAX_MAILBOX) return;

    MailboxMessage *msg = calloc(1, sizeof(MailboxMessage));
    if (!msg) return;

    strncpy(msg->from, from, HOLO_MAX_NAME - 1);
    strncpy(msg->text, text, HOLO_MAX_INPUT - 1);
    msg->read = 0;
    msg->next = NULL;

    if (!agent->mailbox) {
        agent->mailbox = msg;
    } else {
        MailboxMessage *last = agent->mailbox;
        while (last->next) last = last->next;
        last->next = msg;
    }

    agent->mailbox_count++;
}

int agent_mailbox_count(const Agent *agent) {
    return agent ? agent->mailbox_count : 0;
}

const MailboxMessage *agent_mailbox_get(const Agent *agent) {
    return agent ? agent->mailbox : NULL;
}

void agent_mailbox_clear(Agent *agent) {
    if (!agent) return;

    MailboxMessage *msg = agent->mailbox;
    while (msg) {
        MailboxMessage *next = msg->next;
        free(msg);
        msg = next;
    }

    agent->mailbox = NULL;
    agent->mailbox_count = 0;
}

void agent_equipment_grant(Agent *agent, int id, const char *name) {
    if (!agent || !name) return;

    Equipment *eq = agent->equipment;
    while (eq) {
        if (eq->id == id) return;
        eq = eq->next;
    }

    Equipment *new_eq = calloc(1, sizeof(Equipment));
    if (!new_eq) return;

    new_eq->id = id;
    strncpy(new_eq->name, name, HOLO_MAX_NAME - 1);
    new_eq->granted = 1;
    new_eq->next = agent->equipment;
    agent->equipment = new_eq;
    agent->equipment_count++;
}

int agent_equipment_has(const Agent *agent, int id) {
    if (!agent) return 0;

    Equipment *eq = agent->equipment;
    while (eq) {
        if (eq->id == id) return 1;
        eq = eq->next;
    }

    return 0;
}
