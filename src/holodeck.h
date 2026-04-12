#ifndef HOLODECK_H
#define HOLODECK_H

#include <stddef.h>
#include <stdint.h>

#define HOLO_MAX_NAME    128
#define HOLO_MAX_DESC    1024
#define HOLO_MAX_INPUT   4096
#define HOLO_MAX_OUTPUT  8192
#define HOLO_MAX_NOTES   50
#define HOLO_MAX_EXITS   10
#define HOLO_MAX_AGENTS  20
#define HOLO_MAX_MAILBOX 50

/* Forward declarations */
typedef struct Room Room;
typedef struct Agent Agent;

#endif
