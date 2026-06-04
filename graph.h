#ifndef GRAPH_MODULE_GRAPH_H
#define GRAPH_MODULE_GRAPH_H

typedef struct Edge {
    int dest;
    int weight;
    struct Edge* next;
} Edge;

typedef struct Graph {
    int num_vertices;
    Edge** adj_lists;
} Graph;

Graph* create_graph(int num_vertices);
void add_edge(Graph* graph, int src, int dest, int weight);
void print_graph(const Graph* graph);
void free_graph(Graph* graph);
Graph* read_graph_from_file(const char* filename, int* start, int* end);

#endif
