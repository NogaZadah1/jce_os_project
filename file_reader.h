#ifndef FILE_READER_H
#define FILE_READER_H

#include "graph.h"
#include "traveler.h"

Graph* read_graph_from_file(const char* filename, int* start, int* end);

int read_simulation_from_file(
    const char* filename,
    Graph** graph,
    Traveler** travelers,
    int* traveler_count
);

#endif