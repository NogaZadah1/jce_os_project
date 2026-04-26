#include <stdio.h>
#include "graph.h"
#include "file_reader.h"

int main(void) {
    int start, end;

    Graph* graph = read_graph_from_file("input.txt", &start, &end);

    if (graph == NULL) {
        return 1;
    }

    print_graph(graph);
    printf("Start: %d, End: %d\n", start, end);

        return 0;
}
