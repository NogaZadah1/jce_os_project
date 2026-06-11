#ifndef NODE_SYNC_H
#define NODE_SYNC_H

#include <semaphore.h>

typedef struct {
    int node_count;
    sem_t** node_semaphores;
    char** semaphore_names;

    /*
     * Shared debug/validation array.
     * node_occupancy[i] counts how many processes are currently inside node i.
     * It must never be greater than 1.
     */
    int* node_occupancy;
} NodeSync;

/*
 * Creates one named semaphore for each graph node.
 * Each semaphore starts with value 1, meaning the node is available.
 */
NodeSync* node_sync_create(int node_count);

/*
 * Locks a node before a traveler enters it.
 * If another traveler is already inside the node, this call blocks.
 *
 * Returns 0 on success, -1 on failure.
 */
int node_sync_enter(NodeSync* sync, int node_id);

/*
 * Tries to lock a node without blocking.
 * Returns 0 if entered, 1 if the node is busy, -1 on failure.
 */
int node_sync_try_enter(NodeSync* sync, int node_id);

/*
 * Releases a node after the traveler leaves it.
 *
 * Returns 0 on success, -1 on failure.
 */
int node_sync_leave(NodeSync* sync, int node_id);

/*
 * Closes and unlinks all semaphores and releases shared memory.
 */
void node_sync_destroy(NodeSync* sync);

#endif