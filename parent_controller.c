#define _POSIX_C_SOURCE 200809L

#include "parent_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

#include "file_reader.h"
#include "graph.h"
#include "traveler.h"
#include "ipc.h"
#include "m5_gui_adapter.h"

static void print_ipc_message(const IpcMessage* message) {
    if (message == NULL) {
        return;
    }

    if (message->type == IPC_MSG_ARRIVED) {
        if (message->next_node == IPC_DESTINATION_NODE) {
            printf("[PID=%d] arrived at node %d | DESTINATION\n",
                   (int)message->pid,
                   message->current_node);
        } else {
            printf("[PID=%d] arrived at node %d | next node: %d\n",
                   (int)message->pid,
                   message->current_node,
                   message->next_node);
        }
    } else if (message->type == IPC_MSG_FINISHED) {
        printf("[PID=%d] finished\n", (int)message->pid);
    } else if (message->type == IPC_MSG_ERROR) {
        printf("[PID=%d] error\n", (int)message->pid);
    }

    fflush(stdout);
}

static void cleanup_resources(
    Graph* graph,
    Traveler* travelers,
    int (*pipes)[2],
    int traveler_count
) {
    int i;

    if (pipes != NULL) {
        for (i = 0; i < traveler_count; i++) {
            if (pipes[i][0] >= 0) {
                close(pipes[i][0]);
            }

            if (pipes[i][1] >= 0) {
                close(pipes[i][1]);
            }
        }

        free(pipes);
    }

    if (travelers != NULL) {
        free_traveler_paths(travelers, traveler_count);
        free(travelers);
    }

    if (graph != NULL) {
        free_graph(graph);
    }
}

static int create_children(
    const Graph* graph,
    Traveler* travelers,
    int traveler_count,
    int (*pipes)[2]
) {
    int i;

    for (i = 0; i < traveler_count; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 0;
        }

        travelers[i].pid = fork();

        if (travelers[i].pid < 0) {
            perror("fork");
            return 0;
        }

        if (travelers[i].pid == 0) {
            /*
             * Child process:
             * the child writes IPC messages to the parent.
             * The child does not print to the screen.
             */
            close(pipes[i][0]);

            run_child_traveler_m5(
                graph,
                travelers[i].source,
                travelers[i].destination,
                pipes[i][1],
                i
            );

            /*
             * Safety fallback.
             * run_child_traveler_m5 should close write_fd and exit by itself.
             */
            close(pipes[i][1]);
            exit(0);
        }

        /*
         * Parent process:
         * the parent reads IPC messages from the child.
         */
        close(pipes[i][1]);
        pipes[i][1] = -1;
    }

    return 1;
}

static void wait_for_all_children(Traveler* travelers, int traveler_count) {
    int i;

    for (i = 0; i < traveler_count; i++) {
        if (travelers[i].pid > 0) {
            waitpid(travelers[i].pid, NULL, 0);
        }
    }
}

static int read_messages_from_children(
    Traveler* travelers,
    int traveler_count,
    int (*pipes)[2],
    const Graph* graph,
    const Point* node_positions,
    Point* traveler_positions
) {
    int finished_count;
    int i;

    finished_count = 0;

    /*
     * Draw the initial frame before receiving messages.
     */
    m5_gui_render_frame(
        graph,
        node_positions,
        travelers,
        traveler_count,
        traveler_positions
    );

    while (finished_count < traveler_count && !WindowShouldClose()) {
        fd_set read_set;
        int max_fd;
        int ready_count;
        struct timeval timeout;

        FD_ZERO(&read_set);
        max_fd = -1;

        for (i = 0; i < traveler_count; i++) {
            if (!travelers[i].finished && pipes[i][0] >= 0) {
                FD_SET(pipes[i][0], &read_set);

                if (pipes[i][0] > max_fd) {
                    max_fd = pipes[i][0];
                }
            }
        }

        if (max_fd == -1) {
            break;
        }

        /*
         * Short timeout so the GUI stays responsive.
         * Without this, select may block and the window will not render smoothly.
         */
        timeout.tv_sec = 0;
        timeout.tv_usec = 16000; /* about 60 FPS */

        ready_count = select(max_fd + 1, &read_set, NULL, NULL, &timeout);

        if (ready_count < 0) {
            perror("select");
            return 0;
        }

        /*
         * No IPC message arrived in this frame.
         * Render anyway so the window stays open and responsive.
         */
        if (ready_count == 0) {
            m5_gui_render_frame(
                graph,
                node_positions,
                travelers,
                traveler_count,
                traveler_positions
            );
            continue;
        }

        for (i = 0; i < traveler_count; i++) {
            if (!travelers[i].finished &&
                pipes[i][0] >= 0 &&
                FD_ISSET(pipes[i][0], &read_set)) {

                IpcMessage message;
                int read_result;

                read_result = ipc_read_message(pipes[i][0], &message);

                if (read_result < 0) {
                    fprintf(stderr,
                            "Error: failed to read IPC message from traveler %d\n",
                            i);

                    travelers[i].finished = 1;
                    finished_count++;

                    close(pipes[i][0]);
                    pipes[i][0] = -1;

                    continue;
                }

                if (read_result == 0) {
                    /*
                     * Pipe closed.
                     */
                    travelers[i].finished = 1;
                    finished_count++;

                    close(pipes[i][0]);
                    pipes[i][0] = -1;

                    continue;
                }

                print_ipc_message(&message);

                if (message.type == IPC_MSG_ARRIVED) {
                    m5_gui_apply_arrival(
                        &message,
                        node_positions,
                        traveler_positions
                    );
                }

                if (message.type == IPC_MSG_FINISHED ||
                    message.type == IPC_MSG_ERROR) {
                    travelers[i].finished = 1;
                    finished_count++;

                    close(pipes[i][0]);
                    pipes[i][0] = -1;
                }

                m5_gui_render_frame(
                    graph,
                    node_positions,
                    travelers,
                    traveler_count,
                    traveler_positions
                );
            }
        }
    }

    /*
     * Keep the final frame on screen after all travelers finish.
     * The window will close only when the user closes it manually.
     */
    while (!WindowShouldClose()) {
        m5_gui_render_frame(
            graph,
            node_positions,
            travelers,
            traveler_count,
            traveler_positions
        );
    }

    return 1;
}

int run_milestone5(const char* input_file) {
    Graph* graph;
    Traveler* travelers;
    int traveler_count;
    int (*pipes)[2];
    Point* node_positions;
    Point* traveler_positions;
    int i;
    int success;

    graph = NULL;
    travelers = NULL;
    traveler_count = 0;
    pipes = NULL;
    node_positions = NULL;
    traveler_positions = NULL;
    success = 1;

    if (input_file == NULL) {
        fprintf(stderr, "Error: input file is NULL\n");
        return 1;
    }

    if (!read_simulation_from_file(input_file, &graph, &travelers, &traveler_count)) {
        fprintf(stderr, "Error: failed to read simulation file\n");
        return 1;
    }

    if (traveler_count <= 0) {
        fprintf(stderr, "Error: no travelers found\n");
        cleanup_resources(graph, travelers, pipes, traveler_count);
        return 1;
    }

    pipes = malloc((size_t)traveler_count * sizeof(int[2]));
    if (pipes == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        cleanup_resources(graph, travelers, pipes, traveler_count);
        return 1;
    }

    for (i = 0; i < traveler_count; i++) {
        pipes[i][0] = -1;
        pipes[i][1] = -1;
        travelers[i].finished = 0;
        travelers[i].pid = -1;
    }

    if (!m5_gui_init_state(
            graph,
            travelers,
            traveler_count,
            &node_positions,
            &traveler_positions)) {
        fprintf(stderr, "Error: failed to initialize milestone 5 GUI state\n");
        cleanup_resources(graph, travelers, pipes, traveler_count);
        return 1;
    }

    if (!create_children(graph, travelers, traveler_count, pipes)) {
        success = 0;
    }

    if (success) {
        if (!read_messages_from_children(
                travelers,
                traveler_count,
                pipes,
                graph,
                node_positions,
                traveler_positions)) {
            success = 0;
        }
    }

    wait_for_all_children(travelers, traveler_count);

    m5_gui_free_state(node_positions, traveler_positions);

    cleanup_resources(graph, travelers, pipes, traveler_count);

    if (!success) {
        return 1;
    }

    return 0;
}