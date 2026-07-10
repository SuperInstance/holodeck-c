/*
 * test_rooms.c — Room connectivity & lifecycle tests
 *
 * Tests the real, current room API (src/room.h):
 *   room_create / room_destroy
 *   room_connect / room_disconnect / room_find_exit
 *   room_add_agent / room_remove_agent
 *   room_add_note / room_get_notes
 *   room_is_booted / room_set_booted
 *
 * Build standalone:
 *   gcc -std=c99 -Wall -Wextra -Wno-pedantic -Isrc \
 *       -o test_rooms tests/test_rooms.c src/room.c src/agent.c
 */
#include "holodeck.h"
#include "room.h"
#include "agent.h"

#include <stdio.h>
#include <string.h>

static int passed = 0;
static int total = 0;

#define TEST(name) do { total++; printf("  %-40s", #name);
#define PASS(cond) if (cond) { passed++; printf("PASS\n"); } else { printf("FAIL\n"); } } while(0)

/* ════════════════════════════════════════════════════════════════
 *  Room Lifecycle
 * ════════════════════════════════════════════════════════════════ */
static void test_room_create(void) {
    printf("── Room Lifecycle ──\n");

    Room *r = room_create("tavern", "The Tavern", "A cozy place");

    TEST(create_returns_room);
    PASS(r != NULL);

    TEST(create_sets_id);
    PASS(r != NULL && strcmp(r->id, "tavern") == 0);

    TEST(create_sets_name);
    PASS(r != NULL && strcmp(r->name, "The Tavern") == 0);

    TEST(create_sets_description);
    PASS(r != NULL && strcmp(r->description, "A cozy place") == 0);

    TEST(new_room_has_no_exits);
    PASS(room_find_exit(r, "north") == NULL);

    TEST(new_room_has_no_notes);
    PASS(room_get_notes(r) == NULL);

    TEST(new_room_has_no_agents);
    PASS(r->agent_count == 0);

    TEST(new_room_not_booted);
    PASS(room_is_booted(r) == 0);

    room_destroy(r);
}

static void test_room_destroy(void) {
    TEST(destroy_null_safe);
    room_destroy(NULL);
    PASS(1);

    Room *r = room_create("tmp", "Temp", "Temporary");

    TEST(destroy_frees_room);
    room_destroy(r);
    PASS(1);
}

/* ════════════════════════════════════════════════════════════════
 *  Exits / Connectivity
 * ════════════════════════════════════════════════════════════════ */
static void test_room_connect(void) {
    printf("\n── Exits / Connectivity ──\n");

    Room *a = room_create("a", "Room A", "First room");
    Room *b = room_create("b", "Room B", "Second room");

    room_connect(a, b, "north");

    TEST(connect_and_find_exit);
    PASS(room_find_exit(a, "north") == b);

    TEST(find_missing_exit);
    PASS(room_find_exit(a, "south") == NULL);

    TEST(connect_is_oneway);
    PASS(room_find_exit(b, "north") == NULL);

    room_connect(a, b, "south");

    TEST(multiple_exits_findable);
    PASS(room_find_exit(a, "north") == b && room_find_exit(a, "south") == b);

    room_disconnect(a, "north");

    TEST(disconnect_removes_exit);
    PASS(room_find_exit(a, "north") == NULL);

    TEST(disconnect_preserves_other);
    PASS(room_find_exit(a, "south") == b);

    TEST(disconnect_missing_safe);
    room_disconnect(a, "west");
    PASS(1);

    TEST(find_exit_null_safe);
    PASS(room_find_exit(NULL, "north") == NULL);

    TEST(connect_null_safe);
    room_connect(NULL, b, "north");
    room_connect(a, NULL, "north");
    PASS(room_find_exit(a, "north") == NULL && room_find_exit(a, "south") == b);

    room_destroy(a);
    room_destroy(b);
}

/* ════════════════════════════════════════════════════════════════
 *  Agents (enter / leave)
 * ════════════════════════════════════════════════════════════════ */
static void test_room_agents(void) {
    printf("\n── Agents ──\n");

    Room *room = room_create("room", "The Room", "A test room");
    Agent *ag1 = agent_create(-1);
    Agent *ag2 = agent_create(-1);

    room_add_agent(room, ag1);

    TEST(add_agent_increments_count);
    PASS(room->agent_count == 1);

    room_add_agent(room, ag2);

    TEST(add_multiple_agents);
    PASS(room->agent_count == 2);

    room_remove_agent(room, ag1);

    TEST(remove_agent_decrements);
    PASS(room->agent_count == 1);

    room_add_agent(room, ag1);

    TEST(readd_agent);
    PASS(room->agent_count == 2);

    room_remove_agent(room, ag1);

    TEST(remove_absent_agent_safe);
    room_remove_agent(room, ag1);
    PASS(room->agent_count == 1);

    room_remove_agent(room, ag2);
    room_destroy(room);
    agent_destroy(ag1);
    agent_destroy(ag2);

    TEST(add_null_agent_safe);
    room_add_agent(NULL, ag1);
    PASS(1);
}

/* ════════════════════════════════════════════════════════════════
 *  Notes
 * ════════════════════════════════════════════════════════════════ */
static void test_room_notes(void) {
    printf("\n── Notes ──\n");

    Room *room = room_create("notes", "Note Room", "Walls with notes");

    room_add_note(room, "agent1", "Hello world");
    room_add_note(room, "agent2", "Goodbye world");

    const Note *notes = room_get_notes(room);

    TEST(add_note_returns_head);
    PASS(notes != NULL);

    TEST(note_head_is_most_recent);
    PASS(notes != NULL && strcmp(notes->author, "agent2") == 0);

    TEST(note_head_text_correct);
    PASS(notes != NULL && strcmp(notes->text, "Goodbye world") == 0);

    TEST(note_chain_has_previous);
    PASS(notes != NULL && notes->next != NULL &&
         strcmp(notes->next->author, "agent1") == 0);

    TEST(get_notes_null_safe);
    PASS(room_get_notes(NULL) == NULL);

    TEST(add_note_null_safe);
    room_add_note(room, NULL, "no author");
    PASS(1);

    room_destroy(room);
}

/* ════════════════════════════════════════════════════════════════
 *  Boot state
 * ════════════════════════════════════════════════════════════════ */
static void test_room_boot(void) {
    printf("\n── Boot state ──\n");

    Room *room = room_create("rt", "Runtime", "A runtime room");

    TEST(boot_set_true);
    room_set_booted(room, 1);
    PASS(room_is_booted(room) == 1);

    TEST(shutdown_sets_false);
    room_set_booted(room, 0);
    PASS(room_is_booted(room) == 0);

    TEST(is_booted_null_safe);
    PASS(room_is_booted(NULL) == 0);

    TEST(set_booted_null_safe);
    room_set_booted(NULL, 1);
    PASS(1);

    room_destroy(room);
}

int main(void) {
    printf("=== Holodeck C -- Room API Tests ===\n\n");

    test_room_create();
    test_room_destroy();
    test_room_connect();
    test_room_agents();
    test_room_notes();
    test_room_boot();

    printf("\n=== Results: %d/%d passed ===\n", passed, total);

    if (passed == total) {
        printf("Status: ALL PASSED\n");
        return 0;
    } else {
        printf("Status: %d FAILED\n", total - passed);
        return 1;
    }
}
