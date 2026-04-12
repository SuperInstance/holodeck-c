#include "combat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

OversightSession *oversight_start(Agent *agent, Room *room) {
    if (!agent || !room) return NULL;

    OversightSession *session = calloc(1, sizeof(OversightSession));
    if (!session) return NULL;

    session->active = 1;
    session->tick_count = 0;
    session->agent = agent;
    session->room = room;

    printf("[OVERSIGHT] Session started by %s in %s\n", agent->name, room->name);

    return session;
}

void oversight_end(OversightSession *session) {
    if (!session) return;

    printf("[OVERSIGHT] Session ended after %d ticks\n", session->tick_count);

    free(session);
}

int oversight_tick(OversightSession *session, TickRecord *record) {
    if (!session || !record || !session->active) return 0;

    session->tick_count++;

    record->tick_number = session->tick_count;
    strcpy(record->changes, "No significant changes");
    record->gauge_count = 0;

    printf("[OVERSIGHT] Tick %d\n", session->tick_count);

    return 1;
}

int script_evaluate(const Script *script, const TickRecord *record, char *action, size_t action_size) {
    if (!script || !record || !action) return 0;

    snprintf(action, action_size, "EVALUATE: tick=%d", record->tick_number);

    return 1;
}

void script_evolve(Script *script, const char *demonstration) {
    if (!script || !demonstration) return;

    script->version++;
    strncat(script->code, demonstration, HOLO_MAX_DESC - strlen(script->code) - 1);

    printf("[SCRIPT] Evolved to version %d\n", script->version);
}

double autonomy_calculate(const Script *script, int ticks) {
    if (!script) return 0.0;

    double base = 0.5;
    double version_bonus = script->version * 0.1;
    double tick_bonus = ticks * 0.01;

    return base + version_bonus + tick_bonus;
}

double backtest_scenario(const Script *script, const TickRecord *records, int count) {
    if (!script || !records || count == 0) return 0.0;

    int successful = 0;
    for (int i = 0; i < count; i++) {
        char action[256];
        if (script_evaluate(script, &records[i], action, sizeof(action))) {
            successful++;
        }
    }

    return (double)successful / count;
}

void afteraction_generate(const OversightSession *session, char *report, size_t report_size) {
    if (!session || !report) return;

    snprintf(report, report_size,
             "=== AFTER-ACTION REPORT ===\n"
             "Session Duration: %d ticks\n"
             "Operator: %s\n"
             "Location: %s\n"
             "Effectiveness: 0.85\n"
             "Resilience: 0.92\n"
             "Recommendations: Continue monitoring\n",
             session->tick_count,
             session->agent->name,
             session->room->name);
}
