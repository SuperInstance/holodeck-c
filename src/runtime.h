#ifndef RUNTIME_H
#define RUNTIME_H

#include "room.h"
#include "agent.h"
#include "holo.h"

void runtime_boot(Room *room, Agent *agent);
void runtime_shutdown(Room *room, Agent *agent);

typedef struct Gauge {
    char name[HOLO_MAX_NAME];
    double value;
    double min;
    double max;
} Gauge;

void runtime_set_gauge(Room *room, const char *name, double value, double min, double max);
int runtime_get_gauge(Room *room, const char *name, double *value);

#endif
