#include "graph.h"
#include <stdio.h>
#include <stdlib.h>

Graph* create_graph(int num_vertices) {
    Graph* graph;
    int i;

    if (num_vertices <= 0) {
        return NULL;
    }

    graph = malloc(sizeof(Graph));
    if (graph == NULL) {
        return NULL;
    }

    graph->num_vertices = num_vertices;
    graph->adj_lists = malloc(num_vertices * sizeof(Edge*));

    if (graph->adj_lists == NULL) {
        free(graph);
        return NULL;
    }

    for (i = 0; i < num_vertices; i++) {
        graph->adj_lists[i] = NULL;
    }

    return graph;
}

void add_edge(Graph* graph, int src, int dest, int weight) {
    Edge* new_edge;

    if (graph == NULL) {
        return;
    }

    if (src < 0 || src >= graph->num_vertices) {
        return;
    }

    if (dest < 0 || dest >= graph->num_vertices) {
        return;
    }

    if (weight < 0) {
        return;
    }

    new_edge = malloc(sizeof(Edge));
    if (new_edge == NULL) {
        return;
    }

    new_edge->dest = dest;
    new_edge->weight = weight;
    new_edge->next = graph->adj_lists[src];
    graph->adj_lists[src] = new_edge;
}

void print_graph(const Graph* graph) {
    int i;
    Edge* current;

    if (graph == NULL) {
        return;
    }

    for (i = 0; i < graph->num_vertices; i++) {
        printf("%d:", i);
        current = graph->adj_lists[i];

        while (current != NULL) {
            printf(" -> (%d, %d)", current->dest, current->weight);
            current = current->next;
        }

        printf("\n");
    }
}

void free_graph(Graph* graph) {
    int i;
    Edge* current;
    Edge* temp;

    if (graph == NULL) {
        return;
    }

    for (i = 0; i < graph->num_vertices; i++) {
        current = graph->adj_lists[i];

        while (current != NULL) {
            temp = current;
            current = current->next;
            free(temp);
        }
    }

    free(graph->adj_lists);
    free(graph);
}

