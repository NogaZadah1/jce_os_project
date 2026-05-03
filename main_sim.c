#include <raylib.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "dijkstra.h"
#include "graph.h"

#define WINDOW_WIDTH 1220
#define WINDOW_HEIGHT 860
#define NODE_RADIUS 24
#define FOOD_RADIUS 6
#define PACMAN_RADIUS 14
#define PACMAN_SPEED 180.0f

typedef struct {
    float x;
    float y;
} Point;

typedef struct {
    int* nodes;
    int length;
} Path;

static void free_path(Path* path);


static void draw_edge_weight(int x, int y, int weight) {
    const int badge_r = 18;
    const char* text = TextFormat("%d", weight);
    int text_w = MeasureText(text, 22);
    int tx = x - text_w / 2;
    int ty = y - 11;

    DrawCircle(x, y, badge_r + 2, (Color){8, 12, 34, 255});
    DrawCircle(x, y, badge_r, (Color){22, 34, 74, 255});
    DrawCircleLines(x, y, (float)badge_r, (Color){255, 232, 145, 255});

    /* Fake bold text by drawing tiny offsets around center. */
    DrawText(text, tx - 1, ty, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx + 1, ty, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx, ty - 1, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx, ty + 1, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx, ty, 22, (Color){255, 252, 210, 255});
}

/* Function to draw a prominent directional arrow on an edge */
static void draw_arrow(Vector2 from, Vector2 to, Color color) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    
    if (len > 0.001f) {
        float ux = dx / len;
        float uy = dy / len;
        
        /* Position the arrow at 75% of the edge length to prevent overlap with the destination node */
        Vector2 tip = { from.x + dx * 0.75f, from.y + dy * 0.75f };
        
        float arrow_len = 22.0f;   /* Length of the arrowhead */
        float arrow_width = 14.0f; /* Width of the arrowhead */
        
        /* Calculate the two base vertices of the triangle representing the arrowhead */
        Vector2 left = { tip.x - ux * arrow_len + uy * arrow_width, tip.y - uy * arrow_len - ux * arrow_width };
        Vector2 right = { tip.x - ux * arrow_len - uy * arrow_width, tip.y - uy * arrow_len + ux * arrow_width };
        
        /* Draw the triangle twice with different vertex orders to ensure it renders correctly */
        DrawTriangle(tip, left, right, color);
        DrawTriangle(tip, right, left, color);
    }
}

static void draw_ghost_node(Vector2 p, float size, Color body_color, int node_id) {
    float top_r = size * 0.58f;
    float body_w = size * 1.15f;
    float body_h = size * 1.00f;
    float rect_x = p.x - body_w / 2.0f;
    float rect_y = p.y - body_h * 0.35f;
    float eye_r = size * 0.18f;
    float pupil_r = size * 0.08f;
    const char* id_text = TextFormat("%d", node_id);
    int font_size = (int)(size * 0.65f);
    if (font_size < 18) font_size = 18;
    if (font_size > 26) font_size = 26;
    int id_w = MeasureText(id_text, font_size);

    DrawCircleV((Vector2){p.x, p.y - size * 0.38f}, top_r, body_color);
    DrawRectangleRounded(
        (Rectangle){rect_x, rect_y, body_w, body_h},
        0.05f,
        8,
        body_color
    );

    DrawCircleV((Vector2){rect_x + body_w * 0.20f, rect_y + body_h}, size * 0.19f, body_color);
    DrawCircleV((Vector2){rect_x + body_w * 0.50f, rect_y + body_h}, size * 0.19f, body_color);
    DrawCircleV((Vector2){rect_x + body_w * 0.80f, rect_y + body_h}, size * 0.19f, body_color);

    DrawCircleV((Vector2){p.x - size * 0.20f, p.y - size * 0.50f}, eye_r, RAYWHITE);
    DrawCircleV((Vector2){p.x + size * 0.20f, p.y - size * 0.50f}, eye_r, RAYWHITE);
    DrawCircleV((Vector2){p.x - size * 0.16f, p.y - size * 0.47f}, pupil_r, BLUE);
    DrawCircleV((Vector2){p.x + size * 0.24f, p.y - size * 0.47f}, pupil_r, BLUE);

    DrawText(id_text, (int)(p.x - id_w / 2.0f), (int)(p.y + size * 0.17f), font_size, (Color){15, 22, 45, 255});
}

static void draw_pacman(Vector2 p, float radius, float angle_deg) {
    float mouth = 36.0f;
    float eye_angle = angle_deg - 40.0f;
    Vector2 eye_pos = {
        p.x + cosf(eye_angle * DEG2RAD) * radius * 0.35f,
        p.y + sinf(eye_angle * DEG2RAD) * radius * 0.35f
    };

    DrawCircleSector(p, radius, angle_deg + mouth, angle_deg + (360.0f - mouth), 48, YELLOW);
    DrawCircleSectorLines(p, radius, angle_deg + mouth, angle_deg + (360.0f - mouth), 48, GOLD);
    DrawCircleV(eye_pos, radius * 0.12f, BLACK);
}

static Point* build_layout(int n) {
    Point* positions;
    int i;
    float cx = WINDOW_WIDTH / 2.0f;
    float cy = WINDOW_HEIGHT / 2.0f;
    float r = (WINDOW_HEIGHT < WINDOW_WIDTH ? WINDOW_HEIGHT : WINDOW_WIDTH) * 0.40f;

    positions = (Point*)malloc((size_t)n * sizeof(Point));
    if (positions == NULL) {
        return NULL;
    }

    for (i = 0; i < n; i++) {
        float angle = (2.0f * (float)M_PI * (float)i / (float)n) - (float)M_PI / 2.0f;
        positions[i].x = cx + r * cosf(angle);
        positions[i].y = cy + r * sinf(angle);
    }
    return positions;
}

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

static int get_edge_weight(const Graph* graph, int src, int dest) {
    Edge* edge;

    if (graph == NULL || src < 0 || src >= graph->num_vertices) {
        return 1;
    }

    edge = graph->adj_lists[src];
    while (edge != NULL) {
        if (edge->dest == dest) {
            return edge->weight;
        }
        edge = edge->next;
    }

    return 1;
}

static void render_scene(
    const Graph* graph,
    const Point* positions,
    const Path* path,
    const int* food_alive,
    float pacman_x,
    float pacman_y,
    float pacman_angle_deg,
    int is_playing,
    int arrived
) {
    int i;
    Edge* edge;
    static const Color ghost_palette[] = {
        (Color){255, 60, 70, 255},
        (Color){0, 210, 255, 255},
        (Color){255, 190, 50, 255},
        (Color){255, 150, 220, 255},
        (Color){120, 255, 145, 255},
        (Color){170, 130, 255, 255}
    };
    int palette_count = (int)(sizeof(ghost_palette) / sizeof(ghost_palette[0]));

    BeginDrawing();
    ClearBackground((Color){9, 13, 30, 255});

    for (i = 0; i < graph->num_vertices; i++) {
        edge = graph->adj_lists[i];
        while (edge != NULL) {
            Vector2 a = {positions[i].x, positions[i].y};
            Vector2 b = {positions[edge->dest].x, positions[edge->dest].y};
            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float len = sqrtf(dx * dx + dy * dy);
            float nx = 0.0f;
            float ny = 1.0f;
            float side = ((i + edge->dest) % 2 == 0) ? 1.0f : -1.0f;
            float wx;
            float wy;

            if (len > 0.001f) {
                nx = -dy / len;
                ny = dx / len;
            }

            DrawLineEx(a, b, 2.0f, (Color){90, 110, 170, 255});

            wx = (a.x + b.x) * 0.5f + nx * 22.0f * side;
            wy = (a.y + b.y) * 0.5f + ny * 22.0f * side;
            draw_edge_weight((int)wx, (int)wy, edge->weight);
            edge = edge->next;
        }
    }

    if (path->length >= 2) {
        for (i = 0; i < path->length - 1; i++) {
            int a = path->nodes[i];
            int b = path->nodes[i + 1];
            Vector2 p1 = {positions[a].x, positions[a].y};
            Vector2 p2 = {positions[b].x, positions[b].y};
            
            Color path_color = (Color){255, 193, 66, 255};
            DrawLineEx(p1, p2, 4.0f, path_color);
            /* Draw the arrow only on the shortest path */
            draw_arrow(p1, p2, path_color); 
        }
    }

    for (i = 0; i < graph->num_vertices; i++) {
        Color node_color = ghost_palette[i % palette_count];
        draw_ghost_node((Vector2){positions[i].x, positions[i].y}, (float)NODE_RADIUS * 1.25f, node_color, i);
    }

    (void)food_alive;


    Rectangle play_button = { 30, 30, 130, 45 };
    const char* button_text = is_playing ? "Stop" : "Play";

    DrawRectangleRounded(play_button, 0.25f, 8, (Color){30, 90, 150, 255});
    DrawRectangleRoundedLines(play_button, 0.25f, 8, (Color){255, 255, 255, 255});
    DrawText(button_text, (int)(play_button.x + 35), (int)(play_button.y + 12), 22, RAYWHITE);

    if (arrived == 1) {
        DrawText("Arrived at destination!",
                 WINDOW_WIDTH / 2 - 160,
                 30,
                 26,
                 RAYWHITE);
    }

    draw_pacman((Vector2){pacman_x, pacman_y}, (float)PACMAN_RADIUS + 1.5f, pacman_angle_deg);

    EndDrawing();
}

int main(void) {
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

    graph = read_graph_from_file("input.txt", &start, &end);
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