#include <raylib.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "dijkstra.h"
#include "graph.h"
#include "file_reader.h"
#include "gui.h"
#include "traveler.h"

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

int main(int argc, char* argv[]) {
    Graph* graph;
    DijkstraResult* result;
    Path path;
    Path full_route;
    Point* positions = NULL;
    Point* traveler_positions = NULL;
    int* food_alive = NULL;
    Traveler* travelers = NULL;
    int traveler_count = 0;
    int running = 1;
    int segment = 0;
    int* traveler_segments = NULL;
    float* traveler_progress = NULL;
    int* traveler_arrived = NULL;
    int* traveler_waiting = NULL;
    double* traveler_wait_start = NULL;
    int is_playing = 0;
    int arrived = 0;
    int is_waiting = 0;
    double wait_start_time = 0.0;
    float pacman_x = 0.0f;
    float pacman_y = 0.0f;
    float pacman_angle_deg = 0.0f;
    float edge_progress = 0.0f;
    double last_time;

    const char* input_filename;

    if (argc != 2) {
        printf("Usage: ./sim <file_name>\n");
        return 1;
    }

    input_filename = argv[1];

    if (!read_simulation_from_file(
        input_filename,
        &graph,
        &travelers,
        &traveler_count
    )) {
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

    for (int i = 0; i < traveler_count; i++) {
    DijkstraResult* traveler_result = dijkstra(
        graph,
        travelers[i].source,
        travelers[i].destination
    );

    if (traveler_result == NULL) {
        printf("Traveler %d: failed to calculate path\n", i);
        continue;
    }

    if (!build_path_from_dijkstra(&travelers[i], traveler_result)) {
        printf("Traveler %d: no path found\n", i);
        free_dijkstra_result(traveler_result);
        continue;
    }

    printf("Traveler %d path length: %d\n", i, travelers[i].path_length);

    free_dijkstra_result(traveler_result);
    }

    result = dijkstra(
    graph,
    travelers[0].source,
    travelers[0].destination
    );

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
    printf("\nTotal cost: %d\n", result->dist[travelers[0].destination]);

    full_route = build_full_route(graph, travelers[0].source);
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
    traveler_positions = (Point*)malloc((size_t)traveler_count * sizeof(Point));
    traveler_segments = (int*)calloc((size_t)traveler_count, sizeof(int));
    traveler_progress = (float*)calloc((size_t)traveler_count, sizeof(float));
    traveler_arrived = (int*)calloc((size_t)traveler_count, sizeof(int));
    traveler_waiting = (int*)calloc((size_t)traveler_count, sizeof(int));
    traveler_wait_start = (double*)calloc((size_t)traveler_count, sizeof(double));


    if (traveler_segments == NULL ||
        traveler_progress == NULL ||
        traveler_arrived == NULL ||
        traveler_waiting == NULL ||
        traveler_wait_start == NULL) {

        free(traveler_wait_start);
        free(traveler_waiting);
        free(traveler_arrived);
        free(traveler_progress);
        free(traveler_segments);
        free(traveler_positions);

        free(positions);
        free_traveler_paths(travelers, traveler_count);
        free(travelers);
        free_graph(graph);
        return 1;
    }

    if (traveler_positions == NULL) {
        free(positions);
        free_traveler_paths(travelers, traveler_count);
        free(travelers);
        free_graph(graph);
        return 1;
    }

    for (int i = 0; i < traveler_count; i++) {
        traveler_positions[i] = positions[travelers[i].path[0]];
    }

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

    if (!spawn_travelers(travelers, traveler_count)) {
        free(traveler_wait_start);
        free(traveler_waiting);
        free(traveler_arrived);
        free(traveler_progress);
        free(traveler_segments);
        free(traveler_positions);

    free(positions);
    free_traveler_paths(travelers, traveler_count);
    free(travelers);
    free_graph(graph);
    return 1;
    }

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Milestone 4 - Multiple Travelers GUI");
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
        } else if (is_playing == 1) {
            double now = GetTime();
            float dt = (float)(now - last_time);
            int active_count = 0;

            for (int i = 0; i < traveler_count; i++) {
                if (traveler_arrived[i] == 1 || travelers[i].path_length <= 1) {
                    continue;
                }

                if (traveler_waiting[i] == 1) {
                    active_count++;

                    if (GetTime() - traveler_wait_start[i] >= 1.0) {
                        traveler_waiting[i] = 0;
                    }

                    continue;
                }

                if (traveler_segments[i] < travelers[i].path_length - 1) {
                    int src_node = travelers[i].path[traveler_segments[i]];
                    int dst = travelers[i].path[traveler_segments[i] + 1];

                    float sx = positions[src_node].x;
                    float sy = positions[src_node].y;
                    float tx = positions[dst].x;
                    float ty = positions[dst].y;

                    float dx = tx - sx;
                    float dy = ty - sy;

                    int edge_weight = get_edge_weight(graph, src_node, dst);
                    float edge_duration = edge_weight * 0.3f;

                    if (edge_duration <= 0.0f) {
                        edge_duration = 0.3f;
                    }

                    traveler_progress[i] += dt / edge_duration;

                    if (traveler_progress[i] > 1.0f) {
                        traveler_progress[i] = 1.0f;
                    }

                    traveler_positions[i].x = sx + dx * traveler_progress[i];
                    traveler_positions[i].y = sy + dy * traveler_progress[i];

                    if (traveler_progress[i] >= 1.0f) {
                        traveler_positions[i].x = tx;
                        traveler_positions[i].y = ty;

                        traveler_segments[i]++;
                        traveler_progress[i] = 0.0f;

                        if (traveler_segments[i] >= travelers[i].path_length - 1) {
                            traveler_arrived[i] = 1;

                            if (!travelers[i].finished) {
                                kill(travelers[i].pid, SIGTERM);
                                travelers[i].finished = 1;
                            }
                        } else {
                            traveler_waiting[i] = 1;
                            traveler_wait_start[i] = GetTime();
                        }
                    }
                    active_count++;
                } else {
                    traveler_arrived[i] = 1;
                }
            }

            last_time = now;

            if (active_count == 0) {
                arrived = 1;
                is_playing = 0;
            }
        } else {
            last_time = GetTime();
        }

     render_scene(
    graph,
    positions,
    &path,
    food_alive,
    pacman_x,
    pacman_y,
    pacman_angle_deg,
    is_playing,
    arrived,
    travelers,
    traveler_count,
    traveler_positions,
    NULL
    );

    }

    CloseWindow();

    terminate_travelers(travelers, traveler_count);
    wait_for_travelers(travelers, traveler_count);

    free(food_alive);
    free(positions);

    free(traveler_positions);
    free(traveler_segments);
    free(traveler_progress);
    free(traveler_arrived);
    free(traveler_waiting);
    free(traveler_wait_start);

    free_path(&path);
    free_dijkstra_result(result);

    free_traveler_paths(travelers, traveler_count);
    free(travelers);

    free_graph(graph);
    return 0;
}