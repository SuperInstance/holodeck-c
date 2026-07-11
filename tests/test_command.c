/*
 * test_command.c — Command dispatch tests
 *
 * End-to-end coverage for cmd_tell:
 *   - message is delivered to the target agent's mailbox
 *   - sender's own mailbox is untouched
 *   - unknown target produces a sensible error message
 *
 * Build standalone:
 *   gcc -std=c99 -Wall -Wextra -Wno-pedantic -Isrc \
 *       -o test_command tests/test_command.c src/room.c src/agent.c \
 *       src/command.c src/comms.c
 */
#include "holodeck.h"
#include "room.h"
#include "agent.h"
#include "command.h"

#include <stdio.h>
#include <string.h>

static int passed = 0;
static int total = 0;

#define TEST(name) do { total++; printf("  %-40s", #name);
#define PASS(cond) if (cond) { passed++; printf("PASS\n"); } else { printf("FAIL\n"); } } while(0)

static void test_tell_delivers_message(void) {
    printf("── tell command ──\n");

    Room *room = room_create("test", "Test Room", "A room for testing tell");
    Agent *sender = agent_create(-1);
    Agent *receiver = agent_create(-1);

    agent_set_name(sender, "Alice");
    agent_set_name(receiver, "Bob");
    agent_set_room(sender, room);
    agent_set_room(receiver, room);
    room_add_agent(room, sender);
    room_add_agent(room, receiver);

    command_execute(sender, "tell Bob Hello there");

    TEST(tell_reaches_target_mailbox);
    const MailboxMessage *msg = agent_mailbox_get(receiver);
    PASS(msg != NULL &&
         strcmp(msg->from, "Alice") == 0 &&
         strcmp(msg->text, "Hello there") == 0);

    TEST(tell_does_not_reach_sender_mailbox);
    PASS(agent_mailbox_get(sender) == NULL);

    room_remove_agent(room, sender);
    room_remove_agent(room, receiver);
    agent_destroy(sender);
    agent_destroy(receiver);
    room_destroy(room);
}

static void test_tell_unknown_target(void) {
    Room *room = room_create("test2", "Test Room 2", "A room for testing errors");
    Agent *sender = agent_create(-1);

    agent_set_name(sender, "Alice");
    agent_set_room(sender, room);
    room_add_agent(room, sender);

    command_execute(sender, "tell Charlie Hello");

    TEST(tell_unknown_target_reports_error);
    PASS(strstr(sender->output_buffer,
                "don't see anyone by that name here") != NULL);

    room_remove_agent(room, sender);
    agent_destroy(sender);
    room_destroy(room);
}

int main(void) {
    printf("=== Holodeck C -- Command Tests ===\n\n");

    command_register("tell", cmd_tell);

    test_tell_delivers_message();
    test_tell_unknown_target();

    printf("\n=== Results: %d/%d passed ===\n", passed, total);

    if (passed == total) {
        printf("Status: ALL PASSED\n");
        return 0;
    } else {
        printf("Status: %d FAILED\n", total - passed);
        return 1;
    }
}
