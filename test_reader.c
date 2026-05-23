#include <stdio.h>
#include <stdlib.h>

#include "file_reader.h"
#include "traveler.h"
#include "graph.h"

int main() {
    Graph *graph;
    Traveler *travelers;
    int traveler_count;

    if (!read_simulation_from_file(
        "input_m4.txt",
        &graph,
        &travelers,
        &traveler_count
    )) {
        printf("Failed to read simulation file\n");
        return 1;
    }

    printf("Traveler count: %d\n", traveler_count);

    for (int i = 0; i < traveler_count; i++) {
        printf(
            "Traveler %d: %d -> %d\n",
            i,
            travelers[i].source,
            travelers[i].destination
        );
    }

    free(travelers);
    free_graph(graph);

    return 0;
}