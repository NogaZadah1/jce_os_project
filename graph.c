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

Graph* read_graph_from_file(const char* filename, int* start, int* end) {
    FILE* file;
    Graph* graph;
    int n, m;
    int i;
    int src, dst, weight;

    if (filename == NULL || start == NULL || end == NULL) {
        return NULL;
    }

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: could not open file\n");
        return NULL;
    }

    if (fscanf(file, "%d %d", &n, &m) != 2) {
        printf("Error: invalid file format\n");
        fclose(file);
        return NULL;
    }

    if (n <= 0 || m < 0) {
        printf("Error: invalid graph size\n");
        fclose(file);
        return NULL;
    }

    graph = create_graph(n);
    if (graph == NULL) {
        printf("Error: memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    for (i = 0; i < m; i++) {
        if (fscanf(file, "%d %d %d", &src, &dst, &weight) != 3) {
            printf("Error: invalid edge format\n");
            free_graph(graph);
            fclose(file);
            return NULL;
        }

        if (src < 0 || dst < 0 || weight < 0 || src >= n || dst >= n) {
            printf("Error: invalid edge data\n");
            free_graph(graph);
            fclose(file);
            return NULL;
        }

        add_edge(graph, src, dst, weight);
    }

    if (fscanf(file, "%d %d", start, end) != 2) {
        printf("Error: invalid start/end format\n");
        free_graph(graph);
        fclose(file);
        return NULL;
    }

    if (*start < 0 || *end < 0 || *start >= n || *end >= n) {
        printf("Error: invalid start/end vertices\n");
        free_graph(graph);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return graph;
}