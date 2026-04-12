#include "comms.h"
#include "agent.h"
#include "room.h"
#include <string.h>
#include <stdio.h>

void comms_say(Agent *agent, Room *room, const char *message) {
    if (!agent || !room || !message) return;

    char formatted[HOLO_MAX_OUTPUT];
    snprintf(formatted, sizeof(formatted), "%s says: %s\n", agent->name, message);

    for (int i = 0; i < HOLO_MAX_AGENTS; i++) {
        Agent *other = room->agents[i];
        if (other && other != agent) {
            agent_send(other, formatted);
        }
    }
}

void comms_tell(Agent *from, Room *room, Agent *to, const char *message) {
    (void)room;
    if (!from || !to || !message) return;

    agent_mailbox_send(to, from->name, message);
}

void comms_yell(Agent *agent, Room *room, const char *message) {
    if (!agent || !room || !message) return;

    char formatted[HOLO_MAX_OUTPUT];
    snprintf(formatted, sizeof(formatted), "%s yells: %s\n", agent->name, message);

    for (int i = 0; i < HOLO_MAX_AGENTS; i++) {
        Agent *other = room->agents[i];
        if (other && other != agent) {
            agent_send(other, formatted);
        }
    }

    Exit *exit = room->exits;
    while (exit) {
        Room *adjacent = exit->destination;
        if (adjacent) {
            for (int i = 0; i < HOLO_MAX_AGENTS; i++) {
                Agent *listener = adjacent->agents[i];
                if (listener) {
                    agent_send(listener, formatted);
                }
            }
        }
        exit = exit->next;
    }
}

void comms_gossip(Agent *agent, Room *room, const char *message) {
    if (!agent || !message) return;
    (void)room;

    char formatted[HOLO_MAX_OUTPUT];
    snprintf(formatted, sizeof(formatted), "GOSSIP: %s\n", message);
    agent_send(agent, formatted);
}
