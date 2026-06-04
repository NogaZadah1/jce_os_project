#include <stdio.h>
#include "graph.h"
#include "file_reader.h"
#include "dijkstra.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ./dijkstra <file_name>\n");
        return 1;
    }

    int start, end;

    Graph* graph = read_graph_from_file(argv[1], &start, &end);

    if (graph == NULL) {
        return 1;
    }

    DijkstraResult* result = dijkstra(graph, start, end);

    if (result == NULL) {
        free_graph(graph);
        return 1;
    }

    print_dijkstra_result(result);  
    free_dijkstra_result(result);
    free_graph(graph);

    return 0;
}