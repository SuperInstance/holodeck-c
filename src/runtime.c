#include "runtime.h"
#include "manual.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Gauge gauges[100];
    int gauge_count;
} RuntimeData;

void runtime_boot(Room *room, Agent *agent) {
    if (!room || room_is_booted(room)) return;

    printf("[BOOT] Room '%s' booting...\n", room->name);

    RuntimeData *data = calloc(1, sizeof(RuntimeData));
    room->runtime_data = data;

    printf("[BOOT] Step 1: Initialize gauges\n");
    printf("[BOOT] Step 2: Load living manual\n");
    printf("[BOOT] Step 3: Validate safety limits\n");
    printf("[BOOT] Step 4: Start monitoring\n");

    room_set_booted(room, 1);
    printf("[BOOT] Room '%s' boot complete.\n", room->name);

    if (agent) {
        agent_send(agent, "The room hums to life around you.\n");
    }
}

void runtime_shutdown(Room *room, Agent *agent) {
    if (!room || !room_is_booted(room)) return;

    printf("[SHUTDOWN] Room '%s' shutting down...\n", room->name);

    printf("[SHUTDOWN] Saving living manual...\n");
    printf("[SHUTDOWN] Saving gauge readings...\n");
    printf("[SHUTDOWN] Flushing buffers...\n");

    if (room->runtime_data) {
        free(room->runtime_data);
        room->runtime_data = NULL;
    }

    room_set_booted(room, 0);
    printf("[SHUTDOWN] Room '%s' shutdown complete.\n", room->name);

    if (agent) {
        agent_send(agent, "The room powers down with a soft whine.\n");
    }
}

void runtime_set_gauge(Room *room, const char *name, double value, double min, double max) {
    if (!room || !name || !room->runtime_data) return;

    RuntimeData *data = room->runtime_data;
    if (data->gauge_count >= 100) return;

    Gauge *g = &data->gauges[data->gauge_count++];
    strncpy(g->name, name, HOLO_MAX_NAME - 1);
    g->value = value;
    g->min = min;
    g->max = max;
}

int runtime_get_gauge(Room *room, const char *name, double *value) {
    if (!room || !name || !value || !room->runtime_data) return 0;

    RuntimeData *data = room->runtime_data;
    for (int i = 0; i < data->gauge_count; i++) {
        if (strcmp(data->gauges[i].name, name) == 0) {
            *value = data->gauges[i].value;
            return 1;
        }
    }

    return 0;
}
