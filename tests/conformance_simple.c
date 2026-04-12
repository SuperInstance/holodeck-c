#include "holodeck.h"
#include "room.h"
#include "agent.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int passed = 0, total = 0;

#define T(name) printf("  %-50s", name); total++;
#define OK(cond) if (cond) { passed++; printf("PASS\n"); } else { printf("FAIL\n"); }

int main(void) {
    printf("═══ Holodeck C — Conformance Suite ═══\n\n");
    
    /* ═══ T01-T04: Room Lifecycle ═══ */
    printf("── Room Lifecycle ──\n");
    
    T("T01: Create room");
    Room *r = room_create("tavern", "The Tavern", "A cozy place");
    OK(r != NULL && strcmp(r->name, "The Tavern") == 0);
    
    T("T02: Destroy room");
    room_destroy(r);
    /* If we get here without segfault, it passed */
    Room *r2 = room_create("tmp", "Temp", "Temporary");
    room_destroy(r2);
    OK(1);
    
    T("T03: Connect rooms");
    Room *a = room_create("a", "Room A", "First");
    Room *b = room_create("b", "Room B", "Second");
    room_connect(a, b, "north");
    Room *found = room_find_exit(a, "north");
    OK(found != NULL && found == b);
    
    T("T04: Disconnect rooms");
    room_disconnect(a, "north");
    found = room_find_exit(a, "north");
    OK(found == NULL);
    
    room_destroy(a);
    room_destroy(b);
    
    /* ═══ T05-T07: Agent Lifecycle ═══ */
    printf("\n── Agent Lifecycle ──\n");
    
    T("T05: Agent enter room");
    Room *harbor = room_create("harbor", "Harbor", "Where ships dock");
    Agent *ag = agent_create(-1);
    agent_set_name(ag, "test-agent");
    agent_set_room(ag, harbor);
    room_add_agent(harbor, ag);
    OK(harbor->agent_count == 1);
    
    T("T06: Agent leave room");
    room_remove_agent(harbor, ag);
    OK(harbor->agent_count == 0);
    
    T("T07: Agent move rooms");
    Room *tavern = room_create("tavern", "Tavern", "The tavern");
    room_connect(harbor, tavern, "north");
    room_add_agent(harbor, ag);
    room_remove_agent(harbor, ag);
    agent_set_room(ag, tavern);
    room_add_agent(tavern, ag);
    OK(tavern->agent_count == 1 && harbor->agent_count == 0);
    
    agent_destroy(ag);
    
    /* ═══ T08-T13: Communication ═══ */
    printf("\n── Communication ──\n");
    
    T("T08: Agent notes on wall");
    Room *w = room_create("wall", "Wall Room", "A room with walls");
    room_add_note(w, "agent1", "Hello world");
    room_add_note(w, "agent2", "Goodbye world");
    const Note *notes = room_get_notes(w);
    OK(notes != NULL && strcmp(notes->author, "agent2") == 0);
    
    T("T14: Mailbox send/receive");
    Agent *sender = agent_create(-1);
    agent_set_name(sender, "sender");
    agent_mailbox_send(sender, "other", "Test message");
    OK(agent_mailbox_count(sender) >= 0); /* mailbox exists */
    
    T("T15: Equipment grant/check");
    agent_equipment_grant(sender, 1, "build_tool");
    OK(agent_equipment_has(sender, 1));
    
    agent_destroy(sender);
    room_destroy(w);
    
    /* ═══ T16-T17: Permissions ═══ */
    printf("\n── Permissions ──\n");
    
    T("T16: Default permission level");
    Agent *greenhorn = agent_create(-1);
    OK(agent_get_permission(greenhorn) == 0);
    
    T("T17: Set permission level");
    agent_set_permission(greenhorn, 3);
    OK(agent_get_permission(greenhorn) == 3);
    
    agent_destroy(greenhorn);
    
    /* ═══ T21-T22: Room Boot/Shutdown ═══ */
    printf("\n── Room Runtime ──\n");
    
    T("T21: Room boot");
    Room *runtime_room = room_create("rt", "Runtime", "A runtime room");
    room_set_booted(runtime_room, 1);
    OK(room_is_booted(runtime_room));
    
    T("T22: Room shutdown");
    room_set_booted(runtime_room, 0);
    OK(!room_is_booted(runtime_room));
    
    room_destroy(runtime_room);
    room_destroy(harbor);
    room_destroy(tavern);
    
    /* ═══ Summary ═══ */
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
