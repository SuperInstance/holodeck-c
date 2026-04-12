#ifndef COMMAND_H
#define COMMAND_H

#include "agent.h"
#include "holo.h"

typedef struct Command {
    const char *name;
    void (*handler)(Agent *agent, const char *args);
} Command;

void command_register(const char *name, void (*handler)(Agent *agent, const char *args));
int command_execute(Agent *agent, const char *input);
void cmd_look(Agent *agent, const char *args);
void cmd_go(Agent *agent, const char *args);
void cmd_say(Agent *agent, const char *args);
void cmd_tell(Agent *agent, const char *args);
void cmd_yell(Agent *agent, const char *args);
void cmd_gossip(Agent *agent, Room *room, const char *args);
void cmd_note(Agent *agent, const char *args);
void cmd_read(Agent *agent, const char *args);
void cmd_who(Agent *agent, const char *args);
void cmd_quit(Agent *agent, const char *args);
void cmd_help(Agent *agent, const char *args);

#endif
