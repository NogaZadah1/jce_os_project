//
// Created by student on 27/05/2026.
//
#ifndef JCE_OS_PROJECT_MAIN_IPC_H
#define JCE_OS_PROJECT_MAIN_IPC_H

#include <sys/types.h>

#define IPC_DESTINATION_NODE (-1)

typedef enum {
    IPC_MSG_ARRIVED,
    IPC_MSG_FINISHED,
    IPC_MSG_ERROR
} IpcMessageType;

typedef struct {
    IpcMessageType type;
    pid_t pid;
    int traveler_id;
    int current_node;
    int next_node;
} IpcMessage;

int ipc_send_message(int write_fd, const IpcMessage* message);
int ipc_read_message(int read_fd, IpcMessage* message);

#endif //JCE_OS_PROJECT_MAIN_IPC_H