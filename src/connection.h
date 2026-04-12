#ifndef CONNECTION_H
#define CONNECTION_H

#include "holo.h"
#include "agent.h"
#include <stddef.h>

typedef enum {
    CONN_TYPE_HTTP,
    CONN_TYPE_SHELL
} ConnectionType;

typedef struct LiveConnection {
    ConnectionType type;
    char url[HOLO_MAX_INPUT];
    char command[HOLO_MAX_INPUT];
    int active;
    void *data;
} LiveConnection;

LiveConnection *connection_create_http(const char *url);
LiveConnection *connection_create_shell(const char *command);
void connection_destroy(LiveConnection *conn);
int connection_execute(LiveConnection *conn, const char *command, char *output, size_t output_size);

#endif
