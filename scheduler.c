#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>

static int is_valid_node(const Scheduler* scheduler, int node_id) {
    if (scheduler == NULL) {
        return 0;
    }

    if (node_id < 0 || node_id >= scheduler->node_count) {
        return 0;
    }

    return 1;
}

Scheduler* scheduler_create(int node_count) {
    Scheduler* scheduler;
    int i;

    if (node_count <= 0) {
        fprintf(stderr, "scheduler_create: invalid node count\n");
        return NULL;
    }

    scheduler = malloc(sizeof(Scheduler));
    if (scheduler == NULL) {
        return NULL;
    }

    scheduler->node_count = node_count;
    scheduler->next_arrival_order = 0;

    scheduler->node_queues = calloc(
        (size_t)node_count,
        sizeof(NodeQueue)
    );

    if (scheduler->node_queues == NULL) {
        free(scheduler);
        return NULL;
    }

    for (i = 0; i < node_count; i++) {
        scheduler->node_queues[i].head = NULL;
        scheduler->node_queues[i].tail = NULL;
        scheduler->node_queues[i].size = 0;
    }

    return scheduler;
}

int scheduler_enqueue_fcfs(
    Scheduler* scheduler,
    int node_id,
    int traveler_id,
    pid_t pid,
    int next_node
) {
    SchedulerRequest* request;
    NodeQueue* queue;

    if (!is_valid_node(scheduler, node_id)) {
        fprintf(stderr,
                "scheduler_enqueue_fcfs: invalid node id %d\n",
                node_id);
        return -1;
    }

    request = malloc(sizeof(SchedulerRequest));
    if (request == NULL) {
        return -1;
    }

    request->traveler_id = traveler_id;
    request->pid = pid;
    request->node_id = node_id;
    request->next_node = next_node;
    request->arrival_order = scheduler->next_arrival_order;
    request->next = NULL;

    scheduler->next_arrival_order++;

    queue = &scheduler->node_queues[node_id];

    /*
     * FCFS:
     * every new request is inserted at the end of its node queue.
     */
    if (queue->tail == NULL) {
        queue->head = request;
        queue->tail = request;
    } else {
        queue->tail->next = request;
        queue->tail = request;
    }

    queue->size++;

    return 0;
}

int scheduler_dequeue_fcfs(
    Scheduler* scheduler,
    int node_id,
    SchedulerRequest* selected_request
) {
    NodeQueue* queue;
    SchedulerRequest* request;

    if (!is_valid_node(scheduler, node_id) || selected_request == NULL) {
        return -1;
    }

    queue = &scheduler->node_queues[node_id];

    if (queue->head == NULL) {
        return 0;
    }

    /*
     * FCFS:
     * the traveler at the front of the queue entered first,
     * so this traveler gets permission first.
     */
    request = queue->head;

    queue->head = request->next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }

    queue->size--;

    *selected_request = *request;
    selected_request->next = NULL;

    free(request);

    return 1;
}

int scheduler_queue_size(
    const Scheduler* scheduler,
    int node_id
) {
    if (!is_valid_node(scheduler, node_id)) {
        return -1;
    }

    return scheduler->node_queues[node_id].size;
}

int scheduler_has_waiting(
    const Scheduler* scheduler,
    int node_id
) {
    int size;

    size = scheduler_queue_size(scheduler, node_id);

    if (size < 0) {
        return -1;
    }

    return size > 0;
}

void scheduler_destroy(Scheduler* scheduler) {
    int i;

    if (scheduler == NULL) {
        return;
    }

    if (scheduler->node_queues != NULL) {
        for (i = 0; i < scheduler->node_count; i++) {
            SchedulerRequest* current;

            current = scheduler->node_queues[i].head;

            while (current != NULL) {
                SchedulerRequest* next;

                next = current->next;
                free(current);
                current = next;
            }
        }
    }

    free(scheduler->node_queues);
    free(scheduler);
}