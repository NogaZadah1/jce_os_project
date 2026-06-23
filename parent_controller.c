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
#include "node_sync.h"
#include "scheduler.h"
#include "gui.h"


static void print_ipc_message(const IpcMessage* message) {
    if (message == NULL) {
        return;
    }

    if (message->type == IPC_MSG_ARRIVED) {
        if (message->next_node == IPC_DESTINATION_NODE) {
            printf("[PID=%d] arrived at destination node %d\n",
                   (int)message->pid,
                   message->current_node);
        } else {
            printf("[PID=%d] arrived at node %d | next node: %d\n",
                   (int)message->pid,
                   message->current_node,
                   message->next_node);
        }
    } else if (message->type == IPC_MSG_WAITING) {
        printf("[PID=%d] waiting outside node %d\n",
               (int)message->pid,
               message->current_node);
    } else if (message->type == IPC_MSG_FINISHED) {
        printf("[PID=%d] finished\n", (int)message->pid);
    } else if (message->type == IPC_MSG_ERROR) {
        printf("[PID=%d] error\n", (int)message->pid);
    }

    fflush(stdout);
}

static int scheduler_select_next(
    Scheduler* scheduler,
    SchedulingAlgorithm algorithm,
    int node_id,
    SchedulerRequest* selected_request
) {
    if (algorithm == SCHEDULING_FCFS) {
        return scheduler_dequeue_fcfs(
            scheduler,
            node_id,
            selected_request
        );
    }

    if (algorithm == SCHEDULING_SJF) {
        return scheduler_dequeue_sjf(
            scheduler,
            node_id,
            selected_request
        );
    }

    return -1;
}

static int send_grant_entry(
    int grant_write_fd,
    const SchedulerRequest* selected_request
) {
    IpcMessage grant_message = {0};

    if (grant_write_fd < 0 || selected_request == NULL) {
        return 0;
    }

    grant_message.type = IPC_MSG_GRANT_ENTRY;
    grant_message.pid = selected_request->pid;
    grant_message.traveler_id = selected_request->traveler_id;
    grant_message.current_node = selected_request->node_id;
    grant_message.next_node = selected_request->next_node;
    grant_message.remaining_cost = selected_request->remaining_cost;

    return ipc_send_message(grant_write_fd, &grant_message) == 0;
}

static int grant_next_for_node(
    Scheduler* scheduler,
    SchedulingAlgorithm algorithm,
    int node_id,
    int* node_owner,
    int traveler_count,
    int (*grant_pipes)[2]
) {
    SchedulerRequest selected_request;
    int select_result;

    if (scheduler == NULL ||
        node_owner == NULL ||
        grant_pipes == NULL ||
        node_id < 0 ||
        node_id >= scheduler->node_count) {
        return 0;
    }

    /*
     * Someone already received permission for this node.
     */
    if (node_owner[node_id] != -1) {
        return 1;
    }

    select_result = scheduler_select_next(
        scheduler,
        algorithm,
        node_id,
        &selected_request
    );

    /*
     * No traveler is currently waiting for this node.
     */
    if (select_result == 0) {
        return 1;
    }

    if (select_result < 0 ||
        selected_request.traveler_id < 0 ||
        selected_request.traveler_id >= traveler_count) {
        return 0;
    }

    /*
     * Mark the node as reserved before sending the grant.
     */
    node_owner[node_id] = selected_request.traveler_id;

    if (!send_grant_entry(
            grant_pipes[selected_request.traveler_id][1],
            &selected_request)) {
        node_owner[node_id] = -1;
        return 0;
    }

    return 1;
}
static int grant_marked_nodes(
    Scheduler* scheduler,
    SchedulingAlgorithm algorithm,
    int* node_owner,
    int node_count,
    int traveler_count,
    int (*grant_pipes)[2],
    const int* nodes_to_schedule
) {
    int node_id;

    if (scheduler == NULL ||
        node_owner == NULL ||
        grant_pipes == NULL ||
        nodes_to_schedule == NULL ||
        node_count <= 0) {
        return 0;
    }

    for (node_id = 0; node_id < node_count; node_id++) {
        if (nodes_to_schedule[node_id]) {
            if (!grant_next_for_node(
                    scheduler,
                    algorithm,
                    node_id,
                    node_owner,
                    traveler_count,
                    grant_pipes)) {
                return 0;
            }
        }
    }

    return 1;
}
static void close_and_free_pipe_array(
    int (*pipes)[2],
    int pipe_count
) {
    int i;

    if (pipes == NULL) {
        return;
    }

    for (i = 0; i < pipe_count; i++) {
        if (pipes[i][0] >= 0) {
            close(pipes[i][0]);
        }

        if (pipes[i][1] >= 0) {
            close(pipes[i][1]);
        }
    }

    free(pipes);
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

/*
 * Creates milestone 6 traveler child processes.
 * Each child receives the shared NodeSync object and uses it to synchronize
 * access to graph nodes while still computing and following its own path.
 */
static int create_children_m6(
    const Graph* graph,
    Traveler* travelers,
    int traveler_count,
    int (*pipes)[2],
    NodeSync* sync
) {
    int i;

    if (sync == NULL) {
        return 0;
    }

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

            run_child_traveler_m6(
                graph,
                travelers[i].source,
                travelers[i].destination,
                pipes[i][1],
                i,
                sync
            );

            /*
             * Safety fallback.
             * run_child_traveler_m6 should close write_fd and exit by itself.
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

static int create_children_m7(
    const Graph* graph,
    Traveler* travelers,
    int traveler_count,
    int (*child_pipes)[2],
    int (*grant_pipes)[2],
    NodeSync* sync
) {
    int i;
    int j;

    if (graph == NULL ||
        travelers == NULL ||
        child_pipes == NULL ||
        grant_pipes == NULL ||
        sync == NULL) {
        return 0;
    }

    /*
     * Create all pipes before forking.
     * This lets every child close pipe ends that do not belong to it.
     */
    for (i = 0; i < traveler_count; i++) {
        if (pipe(child_pipes[i]) == -1) {
            perror("pipe");
            return 0;
        }

        if (pipe(grant_pipes[i]) == -1) {
            perror("pipe");
            return 0;
        }
    }

    for (i = 0; i < traveler_count; i++) {
        travelers[i].pid = fork();

        if (travelers[i].pid < 0) {
            perror("fork");
            return 0;
        }

        if (travelers[i].pid == 0) {
            /*
             * Child keeps only:
             * child_pipes[i][1]  -> send updates to parent
             * grant_pipes[i][0]  -> receive grants from parent
             */
            for (j = 0; j < traveler_count; j++) {
                close(child_pipes[j][0]);

                if (j != i) {
                    close(child_pipes[j][1]);
                }

                close(grant_pipes[j][1]);

                if (j != i) {
                    close(grant_pipes[j][0]);
                }
            }

            run_child_traveler_m7(
                graph,
                travelers[i].source,
                travelers[i].destination,
                child_pipes[i][1],
                grant_pipes[i][0],
                i,
                sync
            );

            close(child_pipes[i][1]);
            close(grant_pipes[i][0]);
            exit(0);
        }
    }

    /*
     * Parent keeps only:
     * child_pipes[i][0] -> receive updates from child
     * grant_pipes[i][1] -> send grants to child
     */
    for (i = 0; i < traveler_count; i++) {
        close(child_pipes[i][1]);
        child_pipes[i][1] = -1;

        close(grant_pipes[i][0]);
        grant_pipes[i][0] = -1;
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

        m5_gui_render_frame(
            graph,
            node_positions,
            travelers,
            traveler_count,
            traveler_positions
        );

        if (!m5_gui_is_playing()) {
            continue;
        }

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

                if (message.type == IPC_MSG_WAITING) {
                    m5_gui_apply_waiting(
                        &message,
                        node_positions,
                        traveler_positions
                    );
                }

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

    m5_gui_set_all_arrived(1);

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
static int read_messages_from_children_m7(
    Traveler* travelers,
    int traveler_count,
    int (*child_pipes)[2],
    int (*grant_pipes)[2],
    const Graph* graph,
    const Point* node_positions,
    Point* traveler_positions,
    Scheduler* scheduler,
    SchedulingAlgorithm algorithm,
    int* node_owner
) {
    int finished_count;
    int i;
    int node_id;
    int* nodes_to_schedule;

    if (travelers == NULL ||
        child_pipes == NULL ||
        grant_pipes == NULL ||
        graph == NULL ||
        node_positions == NULL ||
        traveler_positions == NULL ||
        scheduler == NULL ||
        node_owner == NULL) {
        return 0;
    }

    nodes_to_schedule = calloc(
        (size_t)graph->num_vertices,
        sizeof(int)
    );

    if (nodes_to_schedule == NULL) {
        return 0;
    }

    finished_count = 0;

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

        m5_gui_render_frame(
            graph,
            node_positions,
            travelers,
            traveler_count,
            traveler_positions
        );

        /*
         * Before Play, children are already blocked waiting for grants,
         * but the parent does not schedule anyone yet.
         */
        if (!m5_gui_is_playing()) {
            continue;
        }

        FD_ZERO(&read_set);
        max_fd = -1;

        for (i = 0; i < traveler_count; i++) {
            if (!travelers[i].finished && child_pipes[i][0] >= 0) {
                FD_SET(child_pipes[i][0], &read_set);

                if (child_pipes[i][0] > max_fd) {
                    max_fd = child_pipes[i][0];
                }
            }
        }

        if (max_fd == -1) {
            free(nodes_to_schedule);
            return 0;
        }

        timeout.tv_sec = 0;
        timeout.tv_usec = 16000;

        ready_count = select(
            max_fd + 1,
            &read_set,
            NULL,
            NULL,
            &timeout
        );

        if (ready_count < 0) {
            perror("select");
            free(nodes_to_schedule);
            return 0;
        }

        if (ready_count == 0) {
            continue;
        }

        for (node_id = 0; node_id < graph->num_vertices; node_id++) {
            nodes_to_schedule[node_id] = 0;
        }

        /*
         * First: read every message that is ready in this select cycle.
         * Only after this loop do we choose who receives grants.
         */
        for (i = 0; i < traveler_count; i++) {
            IpcMessage message;
            int read_result;

            if (travelers[i].finished ||
                child_pipes[i][0] < 0 ||
                !FD_ISSET(child_pipes[i][0], &read_set)) {
                continue;
            }

            read_result = ipc_read_message(
                child_pipes[i][0],
                &message
            );

            if (read_result != 1) {
                fprintf(
                    stderr,
                    "Error: IPC connection closed or failed for traveler %d\n",
                    i
                );

                close(child_pipes[i][0]);
                child_pipes[i][0] = -1;

                free(nodes_to_schedule);
                return 0;
            }

            if (message.traveler_id != i ||
                message.current_node < 0 ||
                message.current_node >= graph->num_vertices) {
                fprintf(stderr, "Error: invalid IPC message from traveler %d\n", i);

                free(nodes_to_schedule);
                return 0;
            }

            print_ipc_message(&message);

            if (message.type == IPC_MSG_WAITING) {
                if (scheduler_enqueue_fcfs(
                        scheduler,
                        message.current_node,
                        message.traveler_id,
                        message.pid,
                        message.next_node,
                        message.remaining_cost) != 0) {
                    fprintf(stderr, "Error: failed to enqueue traveler %d\n", i);

                    free(nodes_to_schedule);
                    return 0;
                }

                m5_gui_apply_waiting(
                    &message,
                    node_positions,
                    traveler_positions
                );

                nodes_to_schedule[message.current_node] = 1;
            } else if (message.type == IPC_MSG_ARRIVED) {
                if (node_owner[message.current_node] != i) {
                    fprintf(
                        stderr,
                        "Error: traveler %d released node %d without ownership\n",
                        i,
                        message.current_node
                    );

                    free(nodes_to_schedule);
                    return 0;
                }

                node_owner[message.current_node] = -1;

                m5_gui_apply_arrival(
                    &message,
                    node_positions,
                    traveler_positions
                );

                nodes_to_schedule[message.current_node] = 1;
            } else if (message.type == IPC_MSG_FINISHED) {
                travelers[i].finished = 1;
                finished_count++;

                close(child_pipes[i][0]);
                child_pipes[i][0] = -1;

                if (grant_pipes[i][1] >= 0) {
                    close(grant_pipes[i][1]);
                    grant_pipes[i][1] = -1;
                }
            } else {
                fprintf(
                    stderr,
                    "Error: unexpected IPC message type from traveler %d\n",
                    i
                );

                free(nodes_to_schedule);
                return 0;
            }
        }

        /*
         * Second: now that all ready WAITING messages are already queued,
         * choose one traveler per free node using FCFS or SJF.
         */
        if (!grant_marked_nodes(
                scheduler,
                algorithm,
                node_owner,
                graph->num_vertices,
                traveler_count,
                grant_pipes,
                nodes_to_schedule)) {
            fprintf(stderr, "Error: failed to grant a traveler entry\n");

            free(nodes_to_schedule);
            return 0;
        }
    }

    free(nodes_to_schedule);

    if (finished_count != traveler_count) {
        return 0;
    }

    m5_gui_set_all_arrived(1);

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
/*
 * Runs milestone 6.
 * This version keeps travelers autonomous, but gives all child processes
 * access to the same NodeSync object so node entry becomes a critical section.
 */
int run_milestone6(const char* input_file) {
    Graph* graph;
    Traveler* travelers;
    int traveler_count;
    int (*pipes)[2];
    Point* node_positions;
    Point* traveler_positions;
    NodeSync* sync;
    int i;
    int success;

    graph = NULL;
    travelers = NULL;
    traveler_count = 0;
    pipes = NULL;
    node_positions = NULL;
    traveler_positions = NULL;
    sync = NULL;
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

    sync = node_sync_create(graph->num_vertices);
    if (sync == NULL) {
        fprintf(stderr, "Error: failed to create node synchronization\n");
        cleanup_resources(graph, travelers, pipes, traveler_count);
        return 1;
    }

    pipes = malloc((size_t)traveler_count * sizeof(int[2]));
    if (pipes == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        node_sync_destroy(sync);
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
        fprintf(stderr, "Error: failed to initialize milestone 6 GUI state\n");
        node_sync_destroy(sync);
        cleanup_resources(graph, travelers, pipes, traveler_count);
        return 1;
    }

    if (!create_children_m6(graph, travelers, traveler_count, pipes, sync)) {
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

    node_sync_destroy(sync);

    cleanup_resources(graph, travelers, pipes, traveler_count);

    if (!success) {
        return 1;
    }

    return 0;
}

int run_milestone7(
    const char* input_file,
    SchedulingAlgorithm algorithm
) {
    Graph* graph = NULL;
    Traveler* travelers = NULL;
    int traveler_count = 0;
    int (*child_pipes)[2] = NULL;
    int (*grant_pipes)[2] = NULL;
    Point* node_positions = NULL;
    Point* traveler_positions = NULL;
    NodeSync* sync = NULL;
    Scheduler* scheduler = NULL;
    int* node_owner = NULL;
    int i;
    int success = 1;

    if (input_file == NULL) {
        fprintf(stderr, "Error: input file is NULL\n");
        return 1;
    }

    if (algorithm != SCHEDULING_FCFS &&
        algorithm != SCHEDULING_SJF) {
        fprintf(stderr, "Error: invalid scheduling algorithm\n");
        return 1;
    }


    if (algorithm == SCHEDULING_FCFS) {
        gui_set_status_label("Scheduling: FCFS");
    } else {
        gui_set_status_label("Scheduling: SJF");
    }

    if (!read_simulation_from_file(
            input_file,
            &graph,
            &travelers,
            &traveler_count)) {
        fprintf(stderr, "Error: failed to read simulation file\n");
        return 1;
    }

    if (traveler_count <= 0) {
        fprintf(stderr, "Error: no travelers found\n");
        cleanup_resources(graph, travelers, NULL, traveler_count);
        return 1;
    }

    sync = node_sync_create(graph->num_vertices);
    if (sync == NULL) {
        fprintf(stderr, "Error: failed to create node synchronization\n");
        cleanup_resources(graph, travelers, NULL, traveler_count);
        return 1;
    }

    scheduler = scheduler_create(graph->num_vertices);
    if (scheduler == NULL) {
        fprintf(stderr, "Error: failed to create scheduler\n");
        node_sync_destroy(sync);
        cleanup_resources(graph, travelers, NULL, traveler_count);
        return 1;
    }

    child_pipes = malloc(
        (size_t)traveler_count * sizeof(int[2])
    );

    grant_pipes = malloc(
        (size_t)traveler_count * sizeof(int[2])
    );

    node_owner = malloc(
        (size_t)graph->num_vertices * sizeof(int)
    );

    if (child_pipes == NULL ||
        grant_pipes == NULL ||
        node_owner == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");

        free(node_owner);
        close_and_free_pipe_array(child_pipes, traveler_count);
        close_and_free_pipe_array(grant_pipes, traveler_count);
        scheduler_destroy(scheduler);
        node_sync_destroy(sync);
        cleanup_resources(graph, travelers, NULL, traveler_count);

        return 1;
    }

    for (i = 0; i < traveler_count; i++) {
        child_pipes[i][0] = -1;
        child_pipes[i][1] = -1;

        grant_pipes[i][0] = -1;
        grant_pipes[i][1] = -1;

        travelers[i].finished = 0;
        travelers[i].pid = -1;
    }

    for (i = 0; i < graph->num_vertices; i++) {
        node_owner[i] = -1;
    }

    if (!m5_gui_init_state(
            graph,
            travelers,
            traveler_count,
            &node_positions,
            &traveler_positions)) {
        fprintf(stderr, "Error: failed to initialize GUI state\n");
        success = 0;
    }

    if (success &&
        !create_children_m7(
            graph,
            travelers,
            traveler_count,
            child_pipes,
            grant_pipes,
            sync)) {
        success = 0;
    }

    if (success &&
        !read_messages_from_children_m7(
            travelers,
            traveler_count,
            child_pipes,
            grant_pipes,
            graph,
            node_positions,
            traveler_positions,
            scheduler,
            algorithm,
            node_owner)) {
        success = 0;
    }

    /*
     * If the window was closed or an error occurred, some children may
     * still be blocked waiting for GRANT_ENTRY. Stop them before waitpid.
     */
    if (!success) {
        terminate_travelers(travelers, traveler_count);
    }

    wait_for_all_children(travelers, traveler_count);
        gui_set_status_label(NULL);

    m5_gui_free_state(node_positions, traveler_positions);


    free(node_owner);

    close_and_free_pipe_array(child_pipes, traveler_count);
    close_and_free_pipe_array(grant_pipes, traveler_count);

    scheduler_destroy(scheduler);
    node_sync_destroy(sync);

    cleanup_resources(graph, travelers, NULL, traveler_count);

    return success ? 0 : 1;
}