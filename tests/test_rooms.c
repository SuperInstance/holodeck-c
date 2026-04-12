#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/room.h"

static int passed = 0;
static int total = 0;

#define TEST(name) do { total++; printf("  %-40s", #name);
#define PASS(cond) if (cond) { passed++; printf("PASS\n"); } else { printf("FAIL\n"); } } while(0)

void test_create_room(void) {
    RoomGraph g;
    graph_init(&g);
    TEST(create_room);
    PASS(graph_create_room(&g, "tavern", "Tavern", "A room") == 1);
    
    TEST(reject_duplicate);
    PASS(graph_create_room(&g, "tavern", "Dupe", "Should fail") == 0);
    
    TEST(count_correct);
    PASS(g.count == 1);
}

void test_destroy_room(void) {
    RoomGraph g;
    graph_init(&g);
    graph_create_room(&g, "tavern", "Tavern", "A room");
    
    TEST(destroy_existing);
    PASS(graph_destroy_room(&g, "tavern") == 1);
    
    TEST(destroy_nonexistent);
    PASS(graph_destroy_room(&g, "nope") == 0);
}

void test_connect_rooms(void) {
    RoomGraph g;
    graph_init(&g);
    graph_create_room(&g, "a", "Room A", "First");
    graph_create_room(&g, "b", "Room B", "Second");
    
    TEST(connect_valid);
    PASS(graph_connect(&g, "a", "north", "b") == 1);
    
    TEST(connect_nonexistent);
    PASS(graph_connect(&g, "a", "south", "c") == 0);
    
    TEST(exit_exists);
    Room *r = graph_find_room(&g, "a");
    PASS(r && r->exit_count == 1 && strcmp(r->exits[0].direction, "north") == 0);
}

void test_room_boot_shutdown(void) {
    Room r;
    room_init(&r, "test", "Test", "Testing");
    
    TEST(boot_sets_flag);
    room_boot(&r, "agent1");
    PASS(r.booted == 1);
    
    TEST(boot_sets_agent);
    PASS(strcmp(r.active_agent, "agent1") == 0);
    
    TEST(shutdown_clears);
    room_shutdown(&r);
    PASS(r.booted == 0);
}

void test_room_notes(void) {
    Room r;
    room_init(&r, "test", "Test", "Testing");
    
    room_add_note(&r, "agent1", "Hello");
    room_add_note(&r, "agent2", "World");
    
    TEST(note_count);
    PASS(r.note_count == 2);
    
    TEST(note_content);
    PASS(strcmp(r.notes[0].author, "agent1") == 0 && strcmp(r.notes[0].content, "Hello") == 0);
}

void test_room_look(void) {
    Room r;
    room_init(&r, "tavern", "The Tavern", "A cozy place");
    room_add_exit(&r, "north", "kitchen");
    char buf[1024];
    room_look(&r, buf, sizeof(buf));
    
    TEST(look_has_name);
    PASS(strstr(buf, "The Tavern") != NULL);
    
    TEST(look_has_exit);
    PASS(strstr(buf, "north") != NULL);
}

void test_remove_exit(void) {
    Room r;
    room_init(&r, "test", "Test", "Testing");
    room_add_exit(&r, "north", "a");
    room_add_exit(&r, "south", "b");
    
    TEST(remove_existing);
    PASS(room_remove_exit(&r, "north") == 1 && r.exit_count == 1);
    
    TEST(remove_nonexistent);
    PASS(room_remove_exit(&r, "west") == 0);
}

int main(void) {
    printf("═══ Holodeck C — Conformance Tests ═══\n\n");
    
    test_create_room();
    test_destroy_room();
    test_connect_rooms();
    test_room_boot_shutdown();
    test_room_notes();
    test_room_look();
    test_remove_exit();
    
    printf("\n═══ Results: %d/%d passed ═══\n", passed, total);
    return passed == total ? 0 : 1;
}
