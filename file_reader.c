#include "file_reader.h"
#include <stdlib.h>
#include <stdio.h>

Graph* read_graph_from_file(const char* filename, int* start, int* end) {
    FILE* file;
    Graph* graph;
    int n, m;
    int i;
    int src, dst, weight;

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

        if (n < 0 || m < 0) {
        printf("Error: negative input is not allowed\n");
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

int read_simulation_from_file(
    const char* filename,
    Graph** graph,
    Traveler** travelers,
    int* traveler_count
) {
    FILE* file;
    int n, m;
    int i;
    int src, dst, weight;

    file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: could not open file\n");
        return 0;
    }

    if (fscanf(file, "%d %d", &n, &m) != 2) {
        printf("Error: invalid file format\n");
        fclose(file);
        return 0;
    }

    if (n <= 0 || m < 0) {
        printf("Error: invalid graph size\n");
        fclose(file);
        return 0;
    }

    *graph = create_graph(n);

    if (*graph == NULL) {
        printf("Error: memory allocation failed\n");
        fclose(file);
        return 0;
    }

    for (i = 0; i < m; i++) {
        if (fscanf(file, "%d %d %d", &src, &dst, &weight) != 3) {
            printf("Error: invalid edge format\n");
            free_graph(*graph);
            fclose(file);
            return 0;
        }

        if (src < 0 || dst < 0 || weight < 0 || src >= n || dst >= n) {
            printf("Error: invalid edge data\n");
            free_graph(*graph);
            fclose(file);
            return 0;
        }

        add_edge(*graph, src, dst, weight);
    }

    if (fscanf(file, "%d", traveler_count) != 1) {
        printf("Error: invalid travelers count\n");
        free_graph(*graph);
        fclose(file);
        return 0;
    }

    if (*traveler_count <= 0) {
        printf("Error: travelers count must be positive\n");
        free_graph(*graph);
        fclose(file);
        return 0;
    }

    *travelers = (Traveler*)malloc((*traveler_count) * sizeof(Traveler));

    if (*travelers == NULL) {
        printf("Error: memory allocation failed\n");
        free_graph(*graph);
        fclose(file);
        return 0;
    }

    for (i = 0; i < *traveler_count; i++) {
        if (fscanf(file, "%d %d", &(*travelers)[i].source, &(*travelers)[i].destination) != 2) {
            printf("Error: invalid traveler format\n");
            free(*travelers);
            free_graph(*graph);
            fclose(file);
            return 0;
        }

        if ((*travelers)[i].source < 0 ||
            (*travelers)[i].destination < 0 ||
            (*travelers)[i].source >= n ||
            (*travelers)[i].destination >= n) {
            printf("Error: invalid traveler data\n");
            free(*travelers);
            free_graph(*graph);
            fclose(file);
            return 0;
        }

        (*travelers)[i].path = NULL;
        (*travelers)[i].path_length = 0;
        (*travelers)[i].pid = -1;
        (*travelers)[i].finished = 0;
    }

    fclose(file);
    return 1;
}
