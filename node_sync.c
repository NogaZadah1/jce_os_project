#define _POSIX_C_SOURCE 200809L

#include "node_sync.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#define NODE_SEM_NAME_SIZE 128

static char* create_semaphore_name(int node_id) {
    char* name;
    int written;

    name = malloc(NODE_SEM_NAME_SIZE * sizeof(char));
    if (name == NULL) {
        return NULL;
    }

    /*
     * POSIX named semaphore names must start with '/'.
     * We include the creator PID to avoid collisions between separate runs.
     */
    written = snprintf(
        name,
        NODE_SEM_NAME_SIZE,
        "/os_project_node_%d_%d",
        (int)getpid(),
        node_id
    );

    if (written < 0 || written >= NODE_SEM_NAME_SIZE) {
        free(name);
        return NULL;
    }

    return name;
}

static int is_valid_node(const NodeSync* sync, int node_id) {
    if (sync == NULL) {
        return 0;
    }

    if (node_id < 0 || node_id >= sync->node_count) {
        return 0;
    }

    return 1;
}

static int create_shared_occupancy(NodeSync* sync, int node_count) {
    size_t size;

    if (sync == NULL || node_count <= 0) {
        return -1;
    }

    size = (size_t)node_count * sizeof(int);

    /*
     * This array is shared between forked processes.
     * It is used as a runtime safety check:
     * after entering a node, occupancy must never be greater than 1.
     */
    sync->node_occupancy = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (sync->node_occupancy == MAP_FAILED) {
        perror("mmap");
        sync->node_occupancy = NULL;
        return -1;
    }

    for (int i = 0; i < node_count; i++) {
        sync->node_occupancy[i] = 0;
    }

    return 0;
}

NodeSync* node_sync_create(int node_count) {
    NodeSync* sync;
    int i;

    if (node_count <= 0) {
        fprintf(stderr, "node_sync_create: invalid node count\n");
        return NULL;
    }

    sync = malloc(sizeof(NodeSync));
    if (sync == NULL) {
        return NULL;
    }

    sync->node_count = node_count;
    sync->node_semaphores = NULL;
    sync->semaphore_names = NULL;
    sync->node_occupancy = NULL;

    sync->node_semaphores = calloc((size_t)node_count, sizeof(sem_t*));
    sync->semaphore_names = calloc((size_t)node_count, sizeof(char*));

    if (sync->node_semaphores == NULL || sync->semaphore_names == NULL) {
        node_sync_destroy(sync);
        return NULL;
    }

    if (create_shared_occupancy(sync, node_count) != 0) {
        node_sync_destroy(sync);
        return NULL;
    }

    for (i = 0; i < node_count; i++) {
        sync->semaphore_names[i] = create_semaphore_name(i);
        if (sync->semaphore_names[i] == NULL) {
            node_sync_destroy(sync);
            return NULL;
        }

        /*
         * Remove stale semaphore with the same name, if it exists.
         */
        sem_unlink(sync->semaphore_names[i]);

        /*
         * Initial value 1:
         * one traveler can enter the node.
         * other travelers must wait outside.
         */
        sync->node_semaphores[i] = sem_open(
            sync->semaphore_names[i],
            O_CREAT | O_EXCL,
            0600,
            1
        );

        if (sync->node_semaphores[i] == SEM_FAILED) {
            perror("sem_open");
            sync->node_semaphores[i] = NULL;
            node_sync_destroy(sync);
            return NULL;
        }
    }

    return sync;
}

int node_sync_enter(NodeSync* sync, int node_id) {
    if (!is_valid_node(sync, node_id)) {
        fprintf(stderr, "node_sync_enter: invalid node id %d\n", node_id);
        return -1;
    }

    /*
     * Critical section entrance:
     * If the node is already occupied, sem_wait blocks here.
     */
    while (sem_wait(sync->node_semaphores[node_id]) == -1) {
        if (errno == EINTR) {
            continue;
        }

        perror("sem_wait");
        return -1;
    }

    /*
     * Runtime validation:
     * because we are after sem_wait, occupancy should become exactly 1.
     * If it becomes greater than 1, synchronization is broken.
     */
    sync->node_occupancy[node_id]++;

    if (sync->node_occupancy[node_id] > 1) {
        fprintf(
            stderr,
            "SYNC ERROR: more than one traveler inside node %d\n",
            node_id
        );
        return -1;
    }

    return 0;
}

/*
 * Tries to enter a node without blocking.
 * If the node is free, the traveler enters it immediately.
 * If the node is already occupied, the function returns 1 without waiting.
 *
 * Returns 0 on success, 1 if the node is busy, -1 on failure.
 */
int node_sync_try_enter(NodeSync* sync, int node_id) {
    if (sync == NULL || node_id < 0 || node_id >= sync->node_count) {
        return -1;
    }

    if (sem_trywait(sync->node_semaphores[node_id]) != 0) {
        if (errno == EAGAIN) {
            return 1;
        }

        perror("sem_trywait");
        return -1;
    }

    sync->node_occupancy[node_id]++;

    if (sync->node_occupancy[node_id] > 1) {
        fprintf(stderr, "SYNC ERROR: more than one traveler inside node %d\n", node_id);
    }

    return 0;
}


int node_sync_leave(NodeSync* sync, int node_id) {
    if (!is_valid_node(sync, node_id)) {
        fprintf(stderr, "node_sync_leave: invalid node id %d\n", node_id);
        return -1;
    }

    /*
     * Runtime validation before leaving.
     */
    if (sync->node_occupancy[node_id] <= 0) {
        fprintf(
            stderr,
            "SYNC ERROR: leaving node %d but occupancy is already %d\n",
            node_id,
            sync->node_occupancy[node_id]
        );
        return -1;
    }

    sync->node_occupancy[node_id]--;

    /*
     * Critical section exit:
     * release the node so another waiting traveler can enter.
     */
    if (sem_post(sync->node_semaphores[node_id]) == -1) {
        perror("sem_post");
        return -1;
    }

    return 0;
}

void node_sync_destroy(NodeSync* sync) {
    int i;

    if (sync == NULL) {
        return;
    }

    if (sync->node_semaphores != NULL) {
        for (i = 0; i < sync->node_count; i++) {
            if (sync->node_semaphores[i] != NULL &&
                sync->node_semaphores[i] != SEM_FAILED) {
                sem_close(sync->node_semaphores[i]);
            }
        }
    }

    if (sync->semaphore_names != NULL) {
        for (i = 0; i < sync->node_count; i++) {
            if (sync->semaphore_names[i] != NULL) {
                sem_unlink(sync->semaphore_names[i]);
                free(sync->semaphore_names[i]);
            }
        }
    }

    if (sync->node_occupancy != NULL) {
        munmap(
            sync->node_occupancy,
            (size_t)sync->node_count * sizeof(int)
        );
    }

    free(sync->node_semaphores);
    free(sync->semaphore_names);
    free(sync);
}