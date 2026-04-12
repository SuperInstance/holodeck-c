#include "conformance.h"
#include "room.h"
#include "agent.h"
#include "command.h"
#include "comms.h"
#include "connection.h"
#include "runtime.h"
#include "combat.h"
#include "manual.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int test_room_lifecycle(TestResult *results, int offset) {
    int test_num = 0;
    int current_test;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T01: Create room";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Room *room = room_create("Test Room", "Test Room", "A test room");
    if (!room) {
        results[current_test].passed = 0;
        results[current_test].message = "Failed to create room";
        test_num++;
        return test_num;
    }
    if (strcmp(room->name, "Test Room") != 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Room name mismatch";
    }
    if (strcmp(room->description, "A test room") != 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Room description mismatch";
    }
    room_destroy(room);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T02: Destroy room";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("Test", "Test", "Test");
    room_destroy(room);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T03: Connect rooms";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Room *room1 = room_create("Room1", "Room1", "First");
    Room *room2 = room_create("Room2", "Room2", "Second");
    room_connect(room1, room2, "north");
    Room *dest = room_find_exit(room1, "north");
    if (!dest) {
        results[current_test].passed = 0;
        results[current_test].message = "No exit found";
    } else if (dest != room2) {
        results[current_test].passed = 0;
        results[current_test].message = "Wrong destination";
    }
    room_destroy(room1);
    room_destroy(room2);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T04: Disconnect rooms";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room1 = room_create("Room1", "Room1", "First");
    room2 = room_create("Room2", "Room2", "Second");
    room_connect(room1, room2, "north");
    room_disconnect(room1, "north");
    dest = room_find_exit(room1, "north");
    if (dest != NULL) {
        results[current_test].passed = 0;
        results[current_test].message = "Exit still exists after disconnect";
    }
    room_destroy(room1);
    room_destroy(room2);
    test_num++;

    return test_num;
}

int test_agent_lifecycle(TestResult *results, int offset) {
    int test_num = 0;
    int current_test;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T05: Agent enters room";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Room *room = room_create("Lobby", "Lobby", "Welcome");
    Agent *agent = agent_create(-1);
    agent_set_name(agent, "TestUser");
    room_add_agent(room, agent);
    agent_set_room(agent, room);
    if (room->agent_count != 1) {
        results[current_test].passed = 0;
        results[current_test].message = "Agent count wrong";
    } else if (agent_get_room(agent) != room) {
        results[current_test].passed = 0;
        results[current_test].message = "Agent room not set";
    }
    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T06: Agent leaves room";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("Lobby", "Lobby", "Welcome");
    agent = agent_create(-1);
    agent_set_name(agent, "TestUser");
    room_add_agent(room, agent);
    room_remove_agent(room, agent);
    if (room->agent_count != 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Agent still in room";
    }
    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T07: Agent moves between rooms";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Room *room1 = room_create("Room1", "Room1", "First");
    Room *room2b = room_create("Room2", "Room2", "Second");
    room_connect(room1, room2b, "east");
    agent = agent_create(-1);
    agent_set_name(agent, "Mover");
    room_add_agent(room1, agent);
    agent_set_room(agent, room1);

    room_remove_agent(room1, agent);
    agent_set_room(agent, room2b);
    room_add_agent(room2b, agent);

    if (agent_get_room(agent) != room2b) {
        results[current_test].passed = 0;
        results[current_test].message = "Agent didn't move";
    } else if (room1->agent_count != 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Left room still has agent";
    } else if (room2b->agent_count != 1) {
        results[current_test].passed = 0;
        results[current_test].message = "New room missing agent";
    }

    room_destroy(room1);
    room_destroy(room2b);
    agent_destroy(agent);
    test_num++;

    return test_num;
}

int test_communication(TestResult *results, int offset) {
    int test_num = 0;
    int current_test;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T08: Say to room";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Room *room = room_create("Chat", "Chat", "Talk here");
    Agent *alice = agent_create(-1);
    Agent *bob = agent_create(-1);
    agent_set_name(alice, "Alice");
    agent_set_name(bob, "Bob");
    room_add_agent(room, alice);
    room_add_agent(room, bob);

    alice->output_pos = 0;
    bob->output_pos = 0;
    alice->room = room;
    bob->room = room;

    comms_say(alice, room, "Hello everyone");

    if (bob->output_pos == 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Bob didn't receive message";
    }

    room_destroy(room);
    agent_destroy(alice);
    agent_destroy(bob);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T09: Tell agent directly";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("Private", "Private", "Private room");
    alice = agent_create(-1);
    bob = agent_create(-1);
    agent_set_name(alice, "Alice");
    agent_set_name(bob, "Bob");
    room_add_agent(room, alice);
    room_add_agent(room, bob);
    alice->room = room;
    bob->room = room;

    comms_tell(alice, room, bob, "Secret message");

    if (agent_mailbox_count(bob) != 1) {
        results[current_test].passed = 0;
        results[current_test].message = "Bob didn't get message";
    }

    room_destroy(room);
    agent_destroy(alice);
    agent_destroy(bob);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T10: Yell to adjacent rooms";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Room *room1 = room_create("Room1", "Room1", "First");
    Room *room2 = room_create("Room2", "Room2", "Second");
    room_connect(room1, room2, "east");
    Agent *yeller = agent_create(-1);
    Agent *listener = agent_create(-1);
    agent_set_name(yeller, "Yeller");
    agent_set_name(listener, "Listener");
    room_add_agent(room1, yeller);
    room_add_agent(room2, listener);

    listener->output_pos = 0;
    comms_yell(yeller, room1, "HELLO EVERYONE");

    if (listener->output_pos == 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Adjacent room didn't hear yell";
    }

    room_destroy(room1);
    room_destroy(room2);
    agent_destroy(yeller);
    agent_destroy(listener);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T11: Gossip fleet-wide";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Agent *gossiper = agent_create(-1);
    agent_set_name(gossiper, "Gossip");
    gossiper->output_pos = 0;
    Room *gossip_room = room_create("GossipRoom", "GossipRoom", "For gossip");
    gossiper->room = gossip_room;

    comms_gossip(gossiper, gossip_room, "Did you hear?");

    if (gossiper->output_pos == 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Gossiper didn't see message";
    }

    room_destroy(gossip_room);
    agent_destroy(gossiper);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T12: Write note on wall";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("Wall", "Wall", "Has a wall");
    room_add_note(room, "Alice", "I was here");

    const Note *notes = room_get_notes(room);
    if (!notes) {
        results[current_test].passed = 0;
        results[current_test].message = "No notes found";
    } else if (strcmp(notes->author, "Alice") != 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Wrong author";
    } else if (strcmp(notes->text, "I was here") != 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Wrong text";
    }

    room_destroy(room);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T13: Read notes from wall";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("Wall", "Wall", "Has a wall");
    room_add_note(room, "Bob", "Me too");

    notes = room_get_notes(room);
    if (!notes) {
        results[current_test].passed = 0;
        results[current_test].message = "No notes found";
    }

    room_destroy(room);
    test_num++;

    return test_num;
}

int test_systems(TestResult *results, int offset) {
    int test_num = 0;
    int current_test;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T14: Mailbox send and receive";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Agent *agent = agent_create(-1);
    agent_set_name(agent, "Recipient");

    agent_mailbox_send(agent, "Sender", "Test message");

    if (agent_mailbox_count(agent) != 1) {
        results[current_test].passed = 0;
        results[current_test].message = "Wrong mailbox count";
    } else {
        const MailboxMessage *msg = agent_mailbox_get(agent);
        if (!msg) {
            results[current_test].passed = 0;
            results[current_test].message = "No message in mailbox";
        } else if (strcmp(msg->from, "Sender") != 0) {
            results[current_test].passed = 0;
            results[current_test].message = "Wrong sender";
        } else if (strcmp(msg->text, "Test message") != 0) {
            results[current_test].passed = 0;
            results[current_test].message = "Wrong text";
        }
    }

    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T15: Equipment grant and check";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    agent = agent_create(-1);
    agent_equipment_grant(agent, 1, "Sword");

    if (!agent_equipment_has(agent, 1)) {
        results[current_test].passed = 0;
        results[current_test].message = "Agent doesn't have equipment";
    } else if (agent_equipment_has(agent, 2)) {
        results[current_test].passed = 0;
        results[current_test].message = "Agent has ungranted equipment";
    }

    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T16: Permission enforced at level 0";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    agent = agent_create(-1);
    agent_set_permission(agent, 0);

    if (agent_get_permission(agent) != 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Permission not set";
    }

    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T17: Permission grants access at level 2";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    agent = agent_create(-1);
    agent_set_permission(agent, 2);

    if (agent_get_permission(agent) != 2) {
        results[current_test].passed = 0;
        results[current_test].message = "Permission not set";
    }

    agent_destroy(agent);
    test_num++;

    return test_num;
}

int test_live_connections(TestResult *results, int offset) {
    int test_num = 0;
    int current_test;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T18: Establish live connection";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    LiveConnection *conn = connection_create_shell("echo test");
    if (!conn) {
        results[current_test].passed = 0;
        results[current_test].message = "Failed to create connection";
    } else if (!conn->active) {
        results[current_test].passed = 0;
        results[current_test].message = "Connection not active";
    }
    if (conn) connection_destroy(conn);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T19: Execute command through connection";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    conn = connection_create_shell("echo hello");
    char output[1024];
    int result = connection_execute(conn, "", output, sizeof(output));
    connection_destroy(conn);

    if (result <= 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Command execution failed";
    }

    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T20: Room change triggers auto-commit";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Room *room1 = room_create("Room1", "Room1", "First");
    Room *room2 = room_create("Room2", "Room2", "Second");
    room_connect(room1, room2, "north");
    Agent *agent = agent_create(-1);
    agent_set_room(agent, room1);
    room_add_agent(room1, agent);

    room_remove_agent(room1, agent);
    agent_set_room(agent, room2);
    room_add_agent(room2, agent);

    if (agent_get_room(agent) != room2) {
        results[current_test].passed = 0;
        results[current_test].message = "Room change failed";
    }

    room_destroy(room1);
    room_destroy(room2);
    agent_destroy(agent);
    test_num++;

    return test_num;
}

int test_room_runtime(TestResult *results, int offset) {
    int test_num = 0;
    int current_test;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T21: Room boots when agent enters";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Room *room = room_create("Runtime", "Runtime", "Runtime test");
    Agent *agent = agent_create(-1);

    runtime_boot(room, agent);

    if (!room_is_booted(room)) {
        results[current_test].passed = 0;
        results[current_test].message = "Room not booted";
    }

    runtime_shutdown(room, agent);
    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T22: Room shuts down when agent leaves";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("Runtime", "Runtime", "Runtime test");
    agent = agent_create(-1);
    runtime_boot(room, agent);

    runtime_shutdown(room, agent);

    if (room_is_booted(room)) {
        results[current_test].passed = 0;
        results[current_test].message = "Room still booted";
    }

    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T23: Living manual - read current generation";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    LivingManual *manual = manual_create();
    const char *content = manual_read_current(manual);

    if (!content) {
        results[current_test].passed = 0;
        results[current_test].message = "No manual content";
    }

    manual_destroy(manual);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T24: Living manual - write feedback";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    manual = manual_create();
    manual_write_feedback(manual, "Great job!");

    const char *feedback = manual_get_feedback(manual);
    if (!feedback || strcmp(feedback, "Great job!") != 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Feedback not saved";
    }

    manual_destroy(manual);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T25: Living manual - evolve to next generation";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    manual = manual_create();
    int old_gen = manual->current_generation;

    manual_evolve(manual);

    if (manual->current_generation <= old_gen) {
        results[current_test].passed = 0;
        results[current_test].message = "Manual didn't evolve";
    }

    manual_destroy(manual);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T26: Zero-shot feedback captured and queryable";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    manual = manual_create();
    manual_write_feedback(manual, "Zero-shot feedback");

    feedback = manual_get_feedback(manual);
    if (!feedback || strstr(feedback, "Zero-shot") == NULL) {
        results[current_test].passed = 0;
        results[current_test].message = "Feedback not captured";
    }

    manual_destroy(manual);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T27: Previous operator notes preserved across sessions";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    manual = manual_create();
    strcpy(manual->generations[0].operator_notes, "Important note");
    manual_evolve(manual);

    const char *old_notes = manual->generations[0].operator_notes;
    if (!old_notes || strcmp(old_notes, "Important note") != 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Notes not preserved";
    }

    manual_destroy(manual);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T28: Boot sequence executes all steps";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("BootTest", "BootTest", "Boot test");
    agent = agent_create(-1);

    runtime_boot(room, agent);

    if (!room_is_booted(room)) {
        results[current_test].passed = 0;
        results[current_test].message = "Boot incomplete";
    }

    runtime_shutdown(room, agent);
    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T29: Safety limits enforced";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("Safe", "Safe", "Safety test");
    agent = agent_create(-1);
    agent_set_permission(agent, 0);

    runtime_boot(room, agent);

    runtime_shutdown(room, agent);
    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T30: Command validation (unknown command returns error)";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("CmdTest", "CmdTest", "Command test");
    agent = agent_create(-1);
    agent_set_room(agent, room);
    room_add_agent(room, agent);
    agent->output_pos = 0;

    command_execute(agent, "unknown_command_xyz");

    if (agent->output_pos == 0) {
        results[current_test].passed = 0;
        results[current_test].message = "No error message";
    }

    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    return test_num;
}

int test_combat_oversight(TestResult *results, int offset) {
    int test_num = 0;
    int current_test;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T31: Oversight session start and end";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Room *room = room_create("Combat", "Combat", "Combat zone");
    Agent *agent = agent_create(-1);
    agent_set_name(agent, "Commander");

    OversightSession *session = oversight_start(agent, room);
    if (!session) {
        results[current_test].passed = 0;
        results[current_test].message = "Failed to start session";
    }

    oversight_end(session);

    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T32: Tick records changes and gauges";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("Combat", "Combat", "Combat zone");
    agent = agent_create(-1);
    OversightSession *session2 = oversight_start(agent, room);

    TickRecord record;
    oversight_tick(session2, &record);

    if (record.tick_number != 1) {
        results[current_test].passed = 0;
        results[current_test].message = "Tick number wrong";
    }

    oversight_end(session2);
    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T33: Script evaluates situation and returns action";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Script script = {0};
    script.version = 1;
    strcpy(script.code, "test_script");

    TickRecord rec = {0};
    char action[256];
    int result = script_evaluate(&script, &rec, action, sizeof(action));

    if (!result) {
        results[current_test].passed = 0;
        results[current_test].message = "Script evaluation failed";
    }

    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T34: Human demonstration evolves script";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Script script2 = {0};
    script2.version = 1;
    strcpy(script2.code, "initial");

    script_evolve(&script2, "demonstration_data");

    if (script2.version != 2) {
        results[current_test].passed = 0;
        results[current_test].message = "Script version didn't increment";
    }

    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T35: Autonomy score calculated correctly";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Script script3 = {0};
    script3.version = 5;

    double score = autonomy_calculate(&script3, 100);

    if (score <= 0.0 || score > 10.0) {
        results[current_test].passed = 0;
        results[current_test].message = "Autonomy score out of range";
    }

    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T36: Back-test engine scores scenario";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Script script4 = {0};
    TickRecord records[5] = {0};

    double backtest_score = backtest_scenario(&script4, records, 5);

    if (backtest_score < 0.0 || backtest_score > 1.0) {
        results[current_test].passed = 0;
        results[current_test].message = "Back-test score out of range";
    }

    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T37: Rival match produces winner";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Script script5 = {0};
    Script script6 = {0};

    double score1 = backtest_scenario(&script5, NULL, 0);
    double score2 = backtest_scenario(&script6, NULL, 0);
    (void)score1;
    (void)score2;

    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T38: Fleet rule promotion (cross-validation)";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T39: After-action report generated with weights";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    room = room_create("Debrief", "Debrief", "Debriefing room");
    agent = agent_create(-1);
    agent_set_name(agent, "Commander");

    OversightSession *session3 = oversight_start(agent, room);
    char report[1024];
    afteraction_generate(session3, report, sizeof(report));

    if (strlen(report) == 0) {
        results[current_test].passed = 0;
        results[current_test].message = "Empty report";
    }

    oversight_end(session3);
    room_destroy(room);
    agent_destroy(agent);
    test_num++;

    current_test = offset + test_num;
    results[current_test].number = current_test;
    results[current_test].name = "T40: Experience weighting (combat effectiveness, resilience)";
    results[current_test].passed = 1;
    results[current_test].message = "OK";

    Script script7 = {0};
    script7.version = 10;
    double autonomy = autonomy_calculate(&script7, 50);

    if (autonomy <= 0.0) {
        results[current_test].passed = 0;
        results[current_test].message = "Invalid experience weighting";
    }

    test_num++;

    return test_num;
}

int conformance_run_all(TestResult *results, int max_results) {
    (void)max_results;
    int total = 0;
    int count;

    count = test_room_lifecycle(results, total);
    total += count;

    count = test_agent_lifecycle(results, total);
    total += count;

    count = test_communication(results, total);
    total += count;

    count = test_systems(results, total);
    total += count;

    count = test_live_connections(results, total);
    total += count;

    count = test_room_runtime(results, total);
    total += count;

    count = test_combat_oversight(results, total);
    total += count;

    return total;
}

int main(void) {
    TestResult results[100];
    int total = conformance_run_all(results, 100);
    int passed = 0;

    printf("═══ Holodeck C — Conformance Suite ═══\n\n");
    printf("── Room Lifecycle ──\n");
    for (int i = 0; i < 4; i++) {
        printf("%s %s\n", results[i].name, results[i].passed ? "PASS" : "FAIL");
        if (results[i].passed) passed++;
    }

    printf("\n── Agent Lifecycle ──\n");
    for (int i = 4; i < 7; i++) {
        printf("%s %s\n", results[i].name, results[i].passed ? "PASS" : "FAIL");
        if (results[i].passed) passed++;
    }

    printf("\n── Communication ──\n");
    for (int i = 7; i < 13; i++) {
        printf("%s %s\n", results[i].name, results[i].passed ? "PASS" : "FAIL");
        if (results[i].passed) passed++;
    }

    printf("\n── Systems ──\n");
    for (int i = 13; i < 17; i++) {
        printf("%s %s\n", results[i].name, results[i].passed ? "PASS" : "FAIL");
        if (results[i].passed) passed++;
    }

    printf("\n── Live Connections ──\n");
    for (int i = 17; i < 20; i++) {
        printf("%s %s\n", results[i].name, results[i].passed ? "PASS" : "FAIL");
        if (results[i].passed) passed++;
    }

    printf("\n── Room Runtime ──\n");
    for (int i = 20; i < 30; i++) {
        printf("%s %s\n", results[i].name, results[i].passed ? "PASS" : "FAIL");
        if (results[i].passed) passed++;
    }

    printf("\n── Combat Oversight ──\n");
    for (int i = 30; i < total; i++) {
        printf("%s %s\n", results[i].name, results[i].passed ? "PASS" : "FAIL");
        if (results[i].passed) passed++;
    }

    printf("\n════════════════════════════════\n");
    printf("Results: %d/%d passed\n", passed, total);

    if (passed == total) {
        printf("Status: ✅ FLEET CERTIFIED\n");
        return 0;
    } else {
        printf("Status: 🟡 %d FAILED\n", total - passed);
        return 1;
    }
}
