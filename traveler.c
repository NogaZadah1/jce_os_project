#include "traveler.h"
#include <stdlib.h>

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