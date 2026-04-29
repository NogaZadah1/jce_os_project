#include "dijkstra.h"
#include <stdio.h>
#include <stdlib.h>

void free_dijkstra_result(DijkstraResult* result) {
    if (result == NULL) {
        return;
    }

    free(result->dist);
    free(result->prev);
    free(result);
}

DijkstraResult* dijkstra(const Graph* graph, int src, int dest) {
    DijkstraResult* result;
    int* visited;
    int i;
    int count;
    int u;
    int v;
    int min_dist;
    Edge* current;

    if (graph == NULL) {
        return NULL;
    }

    if (src < 0 || src >= graph->num_vertices) {
        return NULL;
    }

    if (dest < 0 || dest >= graph->num_vertices) {
        return NULL;
    }

    result = malloc(sizeof(DijkstraResult));
    if (result == NULL) {
        return NULL;
    }

    result->dist = malloc(graph->num_vertices * sizeof(int));
    result->prev = malloc(graph->num_vertices * sizeof(int));
    visited = malloc(graph->num_vertices * sizeof(int));

    if (result->dist == NULL || result->prev == NULL || visited == NULL) {
        free(visited);
        free_dijkstra_result(result);
        return NULL;
    }

    result->num_vertices = graph->num_vertices;
    result->src = src;
    result->dest = dest;

    for (i = 0; i < graph->num_vertices; i++) {
        result->dist[i] = INT_MAX;
        result->prev[i] = -1;
        visited[i] = 0;
    }

    result->dist[src] = 0;

    for (count = 0; count < graph->num_vertices; count++) {
        u = -1;
        min_dist = INT_MAX;

        for (i = 0; i < graph->num_vertices; i++) {
            if (!visited[i] && result->dist[i] < min_dist) {
                min_dist = result->dist[i];
                u = i;
            }
        }

        if (u == -1) {
            break;
        }

        if (u == dest) {
            break;
        }

        visited[u] = 1;
        current = graph->adj_lists[u];

        while (current != NULL) {
            v = current->dest;

            if (!visited[v] && result->dist[u] != INT_MAX &&
                result->dist[u] <= INT_MAX - current->weight &&
                result->dist[u] + current->weight < result->dist[v]) {
                result->dist[v] = result->dist[u] + current->weight;
                result->prev[v] = u;
            }

            current = current->next;
        }
    }

    free(visited);
    return result;
}

void print_dijkstra_result(const DijkstraResult* result) {
    int* path;
    int path_length;
    int current;
    int i;

    if (result == NULL) {
        return;
    }

    if (result->src == result->dest) {
        printf("%d\n", result->src);
        printf("0\n");
        return;
    }

    if (result->dist[result->dest] == INT_MAX) {
        printf("No path found\n");
        return;
    }

    path = malloc(result->num_vertices * sizeof(int));
    if (path == NULL) {
        return;
    }

    path_length = 0;
    current = result->dest;

    while (current != -1) {
        path[path_length] = current;
        path_length++;
        current = result->prev[current];
    }

    for (i = path_length - 1; i >= 0; i--) {
        printf("%d", path[i]);

        if (i > 0) {
            printf(" -> ");
        }
    }

    printf("\n");
    printf("%d\n", result->dist[result->dest]);

    free(path);
}

