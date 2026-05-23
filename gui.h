#ifndef GUI_H
#define GUI_H

#include <raylib.h>
#include "graph.h"
#include "traveler.h"

#define WINDOW_WIDTH 1220
#define WINDOW_HEIGHT 860
#define NODE_RADIUS 24
#define FOOD_RADIUS 6
#define PACMAN_RADIUS 14

typedef struct {
    float x;
    float y;
} Point;

typedef struct {
    int* nodes;
    int length;
} Path;

Point* build_layout(int n);

int get_edge_weight(const Graph* graph, int src, int dest);

void render_scene(
    const Graph* graph,
    const Point* positions,
    const Path* path,
    const int* food_alive,
    float pacman_x,
    float pacman_y,
    float pacman_angle_deg,
    int is_playing,
    int arrived,
    const Traveler* travelers,
    int traveler_count,
    const Point* traveler_positions
);

#endif