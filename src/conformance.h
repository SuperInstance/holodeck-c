#ifndef CONFORMANCE_H
#define CONFORMANCE_H

typedef struct {
    int number;
    const char *name;
    int passed;
    const char *message;
} TestResult;

int conformance_run_all(TestResult *results, int max_results);
int test_room_lifecycle(TestResult *results, int offset);
int test_agent_lifecycle(TestResult *results, int offset);
int test_communication(TestResult *results, int offset);
int test_systems(TestResult *results, int offset);
int test_live_connections(TestResult *results, int offset);
int test_room_runtime(TestResult *results, int offset);
int test_combat_oversight(TestResult *results, int offset);

#endif
