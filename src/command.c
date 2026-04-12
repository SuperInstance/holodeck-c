#include "command.h"
#include "room.h"
#include "comms.h"
#include "holo.h"
#include <string.h>
#include <stdio.h>

#define MAX_COMMANDS 50

static Command commands[MAX_COMMANDS];
static int command_count = 0;

void command_register(const char *name, void (*handler)(Agent *agent, const char *args)) {
    if (command_count >= MAX_COMMANDS) return;
    if (!name || !handler) return;

    commands[command_count].name = name;
    commands[command_count].handler = handler;
    command_count++;
}

int command_execute(Agent *agent, const char *input) {
    if (!agent || !input || !*input) return 0;

    char input_copy[HOLO_MAX_INPUT];
    strncpy(input_copy, input, HOLO_MAX_INPUT - 1);
    input_copy[HOLO_MAX_INPUT - 1] = '\0';

    char *newline = strchr(input_copy, '\n');
    if (newline) *newline = '\0';
    char *cr = strchr(input_copy, '\r');
    if (cr) *cr = '\0';

    char *cmd = strtok(input_copy, " \t");
    char *args = strtok(NULL, "");

    if (!cmd) return 0;

    for (int i = 0; i < command_count; i++) {
        if (strcmp(commands[i].name, cmd) == 0) {
            commands[i].handler(agent, args ? args : "");
            return 1;
        }
    }

    agent_send(agent, "Unknown command. Type 'help' for available commands.\n");
    return 0;
}

void cmd_look(Agent *agent, const char *args) {
    (void)args;
    if (!agent) return;

    Room *room = agent_get_room(agent);
    if (!room) {
        agent_send(agent, "You are nowhere.\n");
        return;
    }

    char buf[HOLO_MAX_OUTPUT];
    snprintf(buf, sizeof(buf), "\n=== %s ===\n%s\n\n", room->name, room->description);
    agent_send(agent, buf);

    if (room->exits) {
        agent_send(agent, "Exits: ");
        Exit *exit = room->exits;
        while (exit) {
            agent_send(agent, exit->direction);
            if (exit->next) agent_send(agent, ", ");
            exit = exit->next;
        }
        agent_send(agent, "\n");
    } else {
        agent_send(agent, "No obvious exits.\n");
    }

    if (room->agent_count > 1) {
        agent_send(agent, "\nAlso here: ");
        int first = 1;
        for (int i = 0; i < HOLO_MAX_AGENTS; i++) {
            Agent *other = room->agents[i];
            if (other && other != agent) {
                if (!first) agent_send(agent, ", ");
                agent_send(agent, other->name);
                first = 0;
            }
        }
        agent_send(agent, "\n");
    }
}

void cmd_go(Agent *agent, const char *args) {
    if (!agent || !args || !*args) {
        agent_send(agent, "Go where?\n");
        return;
    }

    Room *room = agent_get_room(agent);
    if (!room) {
        agent_send(agent, "You are nowhere.\n");
        return;
    }

    Room *dest = room_find_exit(room, args);
    if (!dest) {
        agent_send(agent, "You cannot go that way.\n");
        return;
    }

    room_remove_agent(room, agent);
    agent_set_room(agent, dest);
    room_add_agent(dest, agent);

    cmd_look(agent, "");
}

void cmd_say(Agent *agent, const char *args) {
    if (!agent || !args || !*args) {
        agent_send(agent, "Say what?\n");
        return;
    }

    Room *room = agent_get_room(agent);
    if (!room) return;

    comms_say(agent, room, args);
}

void cmd_tell(Agent *agent, const char *args) {
    if (!agent || !args || !*args) {
        agent_send(agent, "Tell whom what?\n");
        return;
    }

    char args_copy[HOLO_MAX_INPUT];
    strncpy(args_copy, args, HOLO_MAX_INPUT - 1);
    args_copy[HOLO_MAX_INPUT - 1] = '\0';

    char *target = strtok(args_copy, " \t");
    char *message = strtok(NULL, "");

    if (!target || !message) {
        agent_send(agent, "Tell whom what?\n");
        return;
    }

    Room *room = agent_get_room(agent);
    if (!room) return;

    comms_tell(agent, room, target, message);
}

void cmd_yell(Agent *agent, const char *args) {
    if (!agent || !args || !*args) {
        agent_send(agent, "Yell what?\n");
        return;
    }

    Room *room = agent_get_room(agent);
    if (!room) return;

    comms_yell(agent, room, args);
}

void cmd_gossip(Agent *agent, Room *room, const char *args) {
    if (!agent || !args || !*args) {
        agent_send(agent, "Gossip what?\n");
        return;
    }

    comms_gossip(agent, room, args);
}

void cmd_note(Agent *agent, const char *args) {
    if (!agent || !args || !*args) {
        agent_send(agent, "Write what note?\n");
        return;
    }

    Room *room = agent_get_room(agent);
    if (!room) {
        agent_send(agent, "You are nowhere.\n");
        return;
    }

    room_add_note(room, agent->name, args);
    agent_send(agent, "Note written on the wall.\n");
}

void cmd_read(Agent *agent, const char *args) {
    (void)args;
    Room *room = agent_get_room(agent);
    if (!room) {
        agent_send(agent, "You are nowhere.\n");
        return;
    }

    const Note *notes = room_get_notes(room);
    if (!notes) {
        agent_send(agent, "There are no notes here.\n");
        return;
    }

    agent_send(agent, "\n=== Notes on the wall ===\n");
    const Note *note = notes;
    while (note) {
        char buf[HOLO_MAX_OUTPUT];
        snprintf(buf, sizeof(buf), "%s: %s\n\n", note->author, note->text);
        agent_send(agent, buf);
        note = note->next;
    }
}

void cmd_who(Agent *agent, const char *args) {
    (void)args;

    char buf[HOLO_MAX_OUTPUT];
    snprintf(buf, sizeof(buf), "\n=== Who's Here ===\n");

    Room *room = agent_get_room(agent);
    if (room) {
        for (int i = 0; i < HOLO_MAX_AGENTS; i++) {
            Agent *other = room->agents[i];
            if (other) {
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                         "- %s\n", other->name);
            }
        }
    }

    agent_send(agent, buf);
}

void cmd_quit(Agent *agent, const char *args) {
    (void)args;
    agent_send(agent, "Goodbye!\n");
    agent->state = AGENT_STATE_DISCONNECTED;
}

void cmd_help(Agent *agent, const char *args) {
    (void)args;
    agent_send(agent, "\n=== Available Commands ===\n");
    agent_send(agent, "look    - Look around the room\n");
    agent_send(agent, "go <dir>- Move in a direction\n");
    agent_send(agent, "say <msg>- Say something to the room\n");
    agent_send(agent, "tell <name> <msg> - Tell someone something\n");
    agent_send(agent, "yell <msg>- Yell (heard in adjacent rooms)\n");
    agent_send(agent, "gossip <msg> - Gossip to the fleet\n");
    agent_send(agent, "note <msg>- Write a note on the wall\n");
    agent_send(agent, "read    - Read notes on the wall\n");
    agent_send(agent, "who     - See who's connected\n");
    agent_send(agent, "quit    - Disconnect\n");
    agent_send(agent, "help    - Show this help\n");
}
