//
// Created by student on 27/05/2026.
//

#include "ipc.h"

#include <unistd.h>
#include <stddef.h>
#include <errno.h>

int ipc_send_message(int write_fd, const IpcMessage* message) {
    const char* buffer;
    size_t total_written;
    size_t message_size;

    if (message == NULL) {
        return -1;
    }

    buffer = (const char*)message;
    total_written = 0;
    message_size = sizeof(IpcMessage);

    while (total_written < message_size) {
        ssize_t bytes_written = write(
            write_fd,
            buffer + total_written,
            message_size - total_written
        );

        if (bytes_written < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (bytes_written == 0) {
            return -1;
        }

        total_written += bytes_written;
    }

    return 0;
}

int ipc_read_message(int read_fd, IpcMessage* message) {
    char* buffer;
    size_t total_read;
    size_t message_size;

    if (message == NULL) {
        return -1;
    }

    buffer = (char*)message;
    total_read = 0;
    message_size = sizeof(IpcMessage);

    while (total_read < message_size) {
        ssize_t bytes_read = read(
            read_fd,
            buffer + total_read,
            message_size - total_read
        );

        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (bytes_read == 0) {
            if (total_read == 0) {
                return 0;
            }

            return -1;
        }

        total_read += bytes_read;
    }

    return 1;
}