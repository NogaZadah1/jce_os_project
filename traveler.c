
#define _POSIX_C_SOURCE 200809L
#include "traveler.h"
#include "ipc.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int build_path_from_dijkstra(Traveler* traveler, const DijkstraResult* result) {
    int current;
    int count;
    int index;

    if (traveler == NULL || result == NULL) {
        return 0;
    }

    if (result->dist[result->dest] == INT_MAX) {
        traveler->path = NULL;
        traveler->path_length = 0;
        return 0;
    }

    count = 0;
    current = result->dest;

    while (current != -1) {
        count++;
        current = result->prev[current];
    }

    traveler->path = (int*)malloc(count * sizeof(int));

    if (traveler->path == NULL) {
        traveler->path_length = 0;
        return 0;
    }

    traveler->path_length = count;

    current = result->dest;
    index = count - 1;

    while (current != -1) {
        traveler->path[index] = current;
        index--;
        current = result->prev[current];
    }

    return 1;
}

void free_traveler_paths(Traveler* travelers, int traveler_count) {
    int i;

    if (travelers == NULL) {
        return;
    }

    for (i = 0; i < traveler_count; i++) {
        free(travelers[i].path);
        travelers[i].path = NULL;
        travelers[i].path_length = 0;
    }
}

int spawn_travelers(Traveler* travelers, int traveler_count) {
    int i;

    if (travelers == NULL || traveler_count <= 0) {
        return 0;
    }

    for (i = 0; i < traveler_count; i++) {
        travelers[i].pid = fork();

        if (travelers[i].pid < 0) {
            perror("fork failed");
            return 0;
        }

        if (travelers[i].pid == 0) {
            printf("[%d] started\n", getpid());

            while (1) {
                sleep(1);
            }
        }
    }

    return 1;
}

void terminate_travelers(Traveler* travelers, int traveler_count) {
    int i;

    if (travelers == NULL) {
        return;
    }

    for (i = 0; i < traveler_count; i++) {
        if (travelers[i].pid > 0 && !travelers[i].finished) {
            kill(travelers[i].pid, SIGTERM);
            travelers[i].finished = 1;
        }
    }
}

void wait_for_travelers(Traveler* travelers, int traveler_count) {
    int i;

    if (travelers == NULL) {
        return;
    }

    for (i = 0; i < traveler_count; i++) {
        if (travelers[i].pid > 0) {
            waitpid(travelers[i].pid, NULL, 0);
        }
    }
}

void run_child_traveler_m5(
    const Graph* graph,
    int source,
    int destination,
    int write_fd,
    int traveler_id
) {
    DijkstraResult* result;
    Traveler traveler;
    IpcMessage message;
    int i;

    traveler.source = source;
    traveler.destination = destination;
    traveler.path = NULL;
    traveler.path_length = 0;
    traveler.pid = getpid();
    traveler.finished = 0;

    result = dijkstra(graph, source, destination);

    if (result == NULL) {
        close(write_fd);
        exit(1);
    }

    if (!build_path_from_dijkstra(&traveler, result)) {
        free_dijkstra_result(result);
        close(write_fd);
        exit(1);
    }
    for (i = 0; i < traveler.path_length; i++) {
        message.type = IPC_MSG_ARRIVED;
        message.pid = getpid();
        message.traveler_id = traveler_id;
        message.current_node = traveler.path[i];

        if (i + 1 < traveler.path_length) {
            message.next_node = traveler.path[i + 1];
        } else {
            message.next_node = IPC_DESTINATION_NODE;
        }

        if (ipc_send_message(write_fd, &message) != 0) {
            free(traveler.path);
            free_dijkstra_result(result);
            close(write_fd);
            exit(1);
        }
    }
    message.type = IPC_MSG_FINISHED;
    message.pid = getpid();
    message.traveler_id = traveler_id;
    message.current_node = destination;
    message.next_node = IPC_DESTINATION_NODE;

    if (ipc_send_message(write_fd, &message) != 0) {
        free(traveler.path);
        free_dijkstra_result(result);
        close(write_fd);
        exit(1);
    }

    free(traveler.path);
    free_dijkstra_result(result);
    close(write_fd);
    exit(0);
}