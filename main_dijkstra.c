#include <stdio.h>
#include "graph.h"
#include "file_reader.h"

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

    print_graph(graph);
    printf("Start: %d, End: %d\n", start, end);

    free_graph(graph);
    return 0;
}