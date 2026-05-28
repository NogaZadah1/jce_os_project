#ifndef TRAVELER_H
#define TRAVELER_H

#include <sys/types.h>
#include "dijkstra.h"


typedef struct {
    int source;
    int destination;

    int *path;
    int path_length;

    pid_t pid;
    int finished;
} Traveler;

int build_path_from_dijkstra(Traveler* traveler, const DijkstraResult* result);
void free_traveler_paths(Traveler* travelers, int traveler_count);

int spawn_travelers(Traveler* travelers, int traveler_count);
void terminate_travelers(Traveler* travelers, int traveler_count);
void wait_for_travelers(Traveler* travelers, int traveler_count);

void run_child_traveler_m5(
    const Graph* graph,
    int source,
    int destination,
    int write_fd,
    int traveler_id
);

#endif