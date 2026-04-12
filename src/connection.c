#include "connection.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

LiveConnection *connection_create_http(const char *url) {
    if (!url) return NULL;

    LiveConnection *conn = calloc(1, sizeof(LiveConnection));
    if (!conn) return NULL;

    conn->type = CONN_TYPE_HTTP;
    strncpy(conn->url, url, HOLO_MAX_INPUT - 1);
    conn->active = 1;
    conn->data = NULL;

    return conn;
}

LiveConnection *connection_create_shell(const char *command) {
    if (!command) return NULL;

    LiveConnection *conn = calloc(1, sizeof(LiveConnection));
    if (!conn) return NULL;

    conn->type = CONN_TYPE_SHELL;
    strncpy(conn->command, command, HOLO_MAX_INPUT - 1);
    conn->active = 1;
    conn->data = NULL;

    return conn;
}

void connection_destroy(LiveConnection *conn) {
    if (!conn) return;

    if (conn->data) {
        free(conn->data);
    }

    free(conn);
}

int connection_execute(LiveConnection *conn, const char *command, char *output, size_t output_size) {
    if (!conn || !command || !output || output_size == 0) return -1;

    if (conn->type == CONN_TYPE_SHELL) {
        int pipefd[2];
        if (pipe(pipefd) == -1) return -1;

        pid_t pid = fork();
        if (pid == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            return -1;
        }

        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);

            execl("/bin/sh", "sh", "-c", command, NULL);
            exit(1);
        }

        close(pipefd[1]);

        size_t total = 0;
        ssize_t n;
        while ((n = read(pipefd[0], output + total, output_size - total - 1)) > 0) {
            total += n;
            if (total >= output_size - 1) break;
        }
        output[total] = '\0';

        close(pipefd[0]);
        waitpid(pid, NULL, 0);

        return total;
    } else if (conn->type == CONN_TYPE_HTTP) {
        snprintf(output, output_size, "HTTP GET %s: %s\n", conn->url, command);
        return strlen(output);
    }

    return -1;
}
