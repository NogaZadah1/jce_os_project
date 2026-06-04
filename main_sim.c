#include <raylib.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "dijkstra.h"
#include "graph.h"
#include "file_reader.h"
#include "gui.h"

#define WINDOW_WIDTH 1220
#define WINDOW_HEIGHT 860
#define NODE_RADIUS 24
#define FOOD_RADIUS 6
#define PACMAN_RADIUS 14
#define PACMAN_SPEED 180.0f


static void free_path(Path* path);

static Path build_path(const DijkstraResult* result) {
    Path path;
    int current;
    int length = 0;
    int* reversed;
    int i;

    path.nodes = NULL;
    path.length = 0;

    if (result == NULL) {
        return path;
    }

    if (result->dist[result->dest] == INT_MAX) {
        return path;
    }

    reversed = (int*)malloc((size_t)result->num_vertices * sizeof(int));
    if (reversed == NULL) {
        return path;
    }

    current = result->dest;
    while (current != -1) {
        reversed[length++] = current;
        if (current == result->src) {
            break;
        }
        current = result->prev[current];
    }

    if (length == 0 || reversed[length - 1] != result->src) {
        free(reversed);
        return path;
    }

    path.nodes = (int*)malloc((size_t)length * sizeof(int));
    if (path.nodes == NULL) {
        free(reversed);
        return path;
    }

    for (i = 0; i < length; i++) {
        path.nodes[i] = reversed[length - 1 - i];
    }
    path.length = length;

    free(reversed);
    return path;
}

static int append_node(Path* path, int node) {
    int* tmp;

    tmp = (int*)realloc(path->nodes, (size_t)(path->length + 1) * sizeof(int));
    if (tmp == NULL) {
        return 0;
    }
    path->nodes = tmp;
    path->nodes[path->length++] = node;
    return 1;
}

static int append_path_segment(Path* base, const Path* segment, int skip_first) {
    int i;
    for (i = skip_first; i < segment->length; i++) {
        if (!append_node(base, segment->nodes[i])) {
            return 0;
        }
    }
    return 1;
}

static Path build_full_route(const Graph* graph, int start) {
    Path route = {NULL, 0};
    int* eaten = NULL;
    int current = start;
    int i;

    eaten = (int*)calloc((size_t)graph->num_vertices, sizeof(int));
    if (eaten == NULL) {
        return route;
    }

    if (!append_node(&route, start)) {
        free(eaten);
        return route;
    }
    eaten[start] = 1;

    while (1) {
        int best_target = -1;
        int best_dist = INT_MAX;
        for (i = 0; i < graph->num_vertices; i++) {
            if (!eaten[i]) {
                DijkstraResult* probe = dijkstra(graph, current, i);
                if (probe != NULL) {
                    if (probe->dist[i] < best_dist) {
                        best_dist = probe->dist[i];
                        best_target = i;
                    }
                    free_dijkstra_result(probe);
                }
            }
        }

        if (best_target == -1 || best_dist == INT_MAX) {
            break;
        }

        {
            DijkstraResult* leg_result = dijkstra(graph, current, best_target);
            Path leg = build_path(leg_result);

            free_dijkstra_result(leg_result);
            if (leg.length <= 1) {
                free_path(&leg);
                break;
            }
            if (!append_path_segment(&route, &leg, 1)) {
                free_path(&leg);
                break;
            }

            for (i = 0; i < leg.length; i++) {
                eaten[leg.nodes[i]] = 1;
            }
            current = best_target;
            free_path(&leg);
        }
    }

    free(eaten);
    return route;
}

static void free_path(Path* path) {
    if (path == NULL) {
        return;
    }
    free(path->nodes);
    path->nodes = NULL;
    path->length = 0;
}

int main(int argc, char *argv[]) {
    Graph* graph;
    DijkstraResult* result;
    Path path;
    Path full_route;
    Point* positions = NULL;
    int* food_alive = NULL;
    int start, end;
    int running = 1;
    int segment = 0;
    int is_playing = 0;
    int arrived = 0;
    int is_waiting = 0;
    double wait_start_time = 0.0;
    float pacman_x = 0.0f;
    float pacman_y = 0.0f;
    float pacman_angle_deg = 0.0f;
    float edge_progress = 0.0f;
    double last_time;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    graph = read_graph_from_file(argv[1], &start, &end);    
    
    if (graph == NULL) {
            return 1;
    }

    result = dijkstra(graph, start, end);
    if (result == NULL) {
        free_graph(graph);
        return 1;
    }

    path = build_path(result);
    if (path.length == 0) {
        printf("No path found.\n");
        free_dijkstra_result(result);
        free_graph(graph);
        return 1;
    }

    printf("Shortest path: ");
    for (int i = 0; i < path.length; i++) {
        printf("%d", path.nodes[i]);
        if (i < path.length - 1) {
            printf(" -> ");
        }
    }
    printf("\nTotal cost: %d\n", result->dist[end]);

    full_route = build_full_route(graph, start);
    if (full_route.length > 1) {
        free_path(&path);
        path = full_route;
        printf("Full route nodes: ");
        for (int i = 0; i < path.length; i++) {
            printf("%d", path.nodes[i]);
            if (i < path.length - 1) {
                printf(" -> ");
            }
        }
        printf("\n");
    } else {
        free_path(&full_route);
    }

    positions = build_layout(graph->num_vertices);
    if (positions == NULL) {
        free_path(&path);
        free_dijkstra_result(result);
        free_graph(graph);
        return 1;
    }

    food_alive = (int*)malloc((size_t)graph->num_vertices * sizeof(int));
    if (food_alive == NULL) {
        free(positions);
        free_path(&path);
        free_dijkstra_result(result);
        free_graph(graph);
        return 1;
    }
    for (int i = 0; i < graph->num_vertices; i++) {
        food_alive[i] = 1;
    }

    pacman_x = positions[path.nodes[0]].x;
    pacman_y = positions[path.nodes[0]].y;
    food_alive[path.nodes[0]] = 0;

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Milestone 2 - Pacman Graph GUI");
    SetTargetFPS(60);
    last_time = GetTime();

    while (running) {
        if (WindowShouldClose()) {
            running = 0;
        }

        Rectangle play_button = { 30, 30, 130, 45 };

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(GetMousePosition(), play_button) &&
            arrived == 0) {
            is_playing = !is_playing;
            last_time = GetTime();
            }

        if (is_waiting == 1) {
            if (GetTime() - wait_start_time >= 1.0) {
                is_waiting = 0;
                last_time = GetTime();
            }
        } else if (is_playing == 1 && segment < path.length - 1) {
            double now = GetTime();
            float dt = (float)(now - last_time);
            int src_node = path.nodes[segment];
            int dst = path.nodes[segment + 1];

            float sx = positions[src_node].x;
            float sy = positions[src_node].y;
            float tx = positions[dst].x;
            float ty = positions[dst].y;

            float dx = tx - sx;
            float dy = ty - sy;
            float dist = sqrtf(dx * dx + dy * dy);

            int edge_weight = get_edge_weight(graph, src_node, dst);
            float edge_duration = edge_weight * 0.3f;

            if (edge_duration <= 0.0f) {
                edge_duration = 0.3f;
            }

            edge_progress += dt / edge_duration;

            if (edge_progress > 1.0f) {
                edge_progress = 1.0f;
            }

            pacman_x = sx + dx * edge_progress;
            pacman_y = sy + dy * edge_progress;

            last_time = now;

            if (dist > 0.001f) {
                pacman_angle_deg = atan2f(dy, dx) * RAD2DEG;
            }

            if (edge_progress >= 1.0f) {
                pacman_x = tx;
                pacman_y = ty;
                food_alive[dst] = 0;
                segment++;
                edge_progress = 0.0f;

                if (segment >= path.length - 1) {
                    arrived = 1;
                    is_playing = 0;
                    is_waiting = 0;
                } else {
                    is_waiting = 1;
                    wait_start_time = GetTime();
                }
            }
        } else {
            last_time = GetTime();
        }

     render_scene(graph, positions, &path, food_alive, pacman_x, pacman_y, pacman_angle_deg, is_playing, arrived);

    }

    CloseWindow();

    free(food_alive);
    free(positions);
    free_path(&path);
    free_dijkstra_result(result);
    free_graph(graph);
    return 0;
}