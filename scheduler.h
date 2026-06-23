#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <sys/types.h>

/*
 * A single request from a traveler waiting to enter a node.
 */
typedef struct SchedulerRequest {
    int traveler_id;
    pid_t pid;
    int node_id;
    int next_node;

/*
 * Total cost still remaining from node_id until the traveler's destination.
 * Used as the SJF burst value.
 */
int remaining_cost;

    /*
     * Monotonically increasing number assigned when the request enters
     * the scheduler. Used by FCFS to preserve arrival order.
     */
    unsigned long arrival_order;

    struct SchedulerRequest* next;
} SchedulerRequest;

/*
 * One waiting queue per graph node.
 */
typedef struct {
    SchedulerRequest* head;
    SchedulerRequest* tail;
    int size;
} NodeQueue;

/*
 * Parent-owned scheduling state.
 * Each graph node has its own independent waiting queue.
 */
typedef struct {
    int node_count;
    NodeQueue* node_queues;
    unsigned long next_arrival_order;
} Scheduler;

/*
 * Creates one empty FCFS queue for every graph node.
 */
Scheduler* scheduler_create(int node_count);

/*
 * Adds a traveler request to the end of the waiting queue
 * for the requested node.
 *
 * Returns 0 on success, -1 on failure.
 */
int scheduler_enqueue_fcfs(
    Scheduler* scheduler,
    int node_id,
    int traveler_id,
    pid_t pid,
    int next_node,
    int remaining_cost
);

int scheduler_dequeue_fcfs(
    Scheduler* scheduler,
    int node_id,
    SchedulerRequest* selected_request
);

/*
 * Removes and returns the traveler with the smallest remaining_cost
 * from the waiting queue of node_id.
 *
 * If two travelers have the same remaining_cost, the one with the
 * smaller arrival_order is selected first (FCFS tie-break).
 *
 * Returns 1 if a request was returned.
 * Returns 0 if the queue is empty.
 * Returns -1 on invalid input.
 */
int scheduler_dequeue_sjf(
    Scheduler* scheduler,
    int node_id,
    SchedulerRequest* selected_request
);

/*
 * Returns the number of travelers waiting for a specific node.
 * Returns -1 on invalid input.
 */
int scheduler_queue_size(
    const Scheduler* scheduler,
    int node_id
);

/*
 * Returns 1 if at least one traveler is waiting for node_id.
 * Returns 0 if the queue is empty.
 * Returns -1 on invalid input.
 */
int scheduler_has_waiting(
    const Scheduler* scheduler,
    int node_id
);

/*
 * Frees every pending request and the scheduler itself.
 */
void scheduler_destroy(Scheduler* scheduler);

#endif