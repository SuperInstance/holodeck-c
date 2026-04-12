#ifndef MANUAL_H
#define MANUAL_H

#include <stddef.h>
#include "holo.h"

#define MAX_GENERATIONS 100
#define MAX_MANUAL_SIZE 10000

typedef struct {
    int generation;
    char content[MAX_MANUAL_SIZE];
    char zero_shot_feedback[HOLO_MAX_DESC];
    char operator_notes[HOLO_MAX_DESC];
    long timestamp;
} ManualGeneration;

typedef struct {
    ManualGeneration generations[MAX_GENERATIONS];
    int current_generation;
    int generation_count;
} LivingManual;

LivingManual *manual_create(void);
void manual_destroy(LivingManual *manual);
const char *manual_read_current(const LivingManual *manual);
void manual_write_feedback(LivingManual *manual, const char *feedback);
void manual_evolve(LivingManual *manual);
const char *manual_get_feedback(const LivingManual *manual);
const char *manual_get_notes(const LivingManual *manual);

#endif
