#ifndef COMMS_H
#define COMMS_H

#include "holodeck.h"

void comms_say(Agent *agent, Room *room, const char *message);
void comms_tell(Agent *from, Room *room, Agent *to, const char *message);
void comms_yell(Agent *agent, Room *room, const char *message);
void comms_gossip(Agent *agent, Room *room, const char *message);

#endif
