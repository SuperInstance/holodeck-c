/**
 * test_serial_bridge.c — Test harness for serial bridge
 * Build: gcc -std=c99 -Wall -Wextra -Wno-pedantic -Isrc -o test_serial tests/test_serial_bridge.c src/serial_bridge.c
 */
#include "serial_bridge.h"
#include <stdio.h>

int main(void) {
    printf("=== Serial Bridge Tests ===\n");
    int failures = serial_bridge_test();
    if (failures == 0) {
        printf("ALL PASSED\n");
    } else {
        printf("%d FAILURES\n", failures);
    }
    return failures;
}
