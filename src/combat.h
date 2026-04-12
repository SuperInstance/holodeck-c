#ifndef COMBAT_H
#define COMBAT_H

#include "agent.h"
#include "room.h"
#include "holo.h"

typedef struct OversightSession {
    int active;
    int tick_count;
    Agent *agent;
    Room *room;
} OversightSession;

typedef struct TickRecord {
    int tick_number;
    char changes[HOLO_MAX_DESC];
    double gauges[10];
    int gauge_count;
} TickRecord;

typedef struct Script {
    int version;
    char code[HOLO_MAX_DESC];
    double autonomy_score;
} Script;

OversightSession *oversight_start(Agent *agent, Room *room);
void oversight_end(OversightSession *session);
int oversight_tick(OversightSession *session, TickRecord *record);
int script_evaluate(const Script *script, const TickRecord *record, char *action, size_t action_size);
void script_evolve(Script *script, const char *demonstration);
double autonomy_calculate(const Script *script, int ticks);
double backtest_scenario(const Script *script, const TickRecord *records, int count);
void afteraction_generate(const OversightSession *session, char *report, size_t report_size);

#endif
