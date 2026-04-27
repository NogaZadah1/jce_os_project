#ifndef GRAPH_MODULE_DIJKSTRA_H
#define GRAPH_MODULE_DIJKSTRA_H

#include "graph.h"
#include <limits.h>

typedef struct DijkstraResult {
    int* dist;
    int* prev;
    int num_vertices;
    int src;
    int dest;
} DijkstraResult;

DijkstraResult* dijkstra(const Graph* graph, int src, int dest);
void print_dijkstra_result(const DijkstraResult* result);
void free_dijkstra_result(DijkstraResult* result);

#endif
