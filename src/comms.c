#include "comms.h"
#include <string.h>

void comms_say(Agent *agent, Room *room, const char *message) {
    (void)room;
    (void)agent;
    (void)message;
}

void comms_tell(Agent *from, Room *room, Agent *to, const char *message) {
    (void)room; (void)from; (void)to; (void)message;
}

void comms_yell(Agent *agent, Room *room, const char *message) {
    (void)room; (void)agent; (void)message;
}

void comms_gossip(Agent *agent, Room *room, const char *message) {
    (void)room; (void)agent; (void)message;
}
