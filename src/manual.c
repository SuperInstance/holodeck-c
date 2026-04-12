#include "manual.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

LivingManual *manual_create(void) {
    LivingManual *manual = calloc(1, sizeof(LivingManual));
    if (!manual) return NULL;

    manual->current_generation = 0;
    manual->generation_count = 1;

    ManualGeneration *gen = &manual->generations[0];
    gen->generation = 0;
    strcpy(gen->content, "# Holodeck Manual - Generation 0\n\nBasic operations and protocols.\n");
    gen->zero_shot_feedback[0] = '\0';
    gen->operator_notes[0] = '\0';
    gen->timestamp = time(NULL);

    return manual;
}

void manual_destroy(LivingManual *manual) {
    free(manual);
}

const char *manual_read_current(const LivingManual *manual) {
    if (!manual || manual->generation_count == 0) return NULL;

    return manual->generations[manual->current_generation].content;
}

void manual_write_feedback(LivingManual *manual, const char *feedback) {
    if (!manual || !feedback) return;

    ManualGeneration *gen = &manual->generations[manual->current_generation];
    strncpy(gen->zero_shot_feedback, feedback, HOLO_MAX_DESC - 1);
    gen->zero_shot_feedback[HOLO_MAX_DESC - 1] = '\0';

    printf("[MANUAL] Feedback recorded for generation %d\n", gen->generation);
}

void manual_evolve(LivingManual *manual) {
    if (!manual || manual->generation_count >= MAX_GENERATIONS) return;

    int old_gen = manual->current_generation;
    int new_gen = manual->generation_count;

    manual->current_generation = new_gen;
    manual->generation_count++;

    ManualGeneration *gen = &manual->generations[new_gen];
    gen->generation = new_gen;
    gen->timestamp = time(NULL);

    snprintf(gen->content, MAX_MANUAL_SIZE,
             "# Holodeck Manual - Generation %d\n\n"
             "Evolved from generation %d.\n\n"
             "Previous feedback: %s\n\n"
             "Improved protocols and procedures.\n",
             new_gen, old_gen,
             manual->generations[old_gen].zero_shot_feedback);

    gen->zero_shot_feedback[0] = '\0';
    gen->operator_notes[0] = '\0';

    printf("[MANUAL] Evolved to generation %d\n", new_gen);
}

const char *manual_get_feedback(const LivingManual *manual) {
    if (!manual) return NULL;

    return manual->generations[manual->current_generation].zero_shot_feedback;
}

const char *manual_get_notes(const LivingManual *manual) {
    if (!manual) return NULL;

    return manual->generations[manual->current_generation].operator_notes;
}
