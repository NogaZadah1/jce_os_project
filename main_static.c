#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "dijkstra.h"
#include "graph.h"
#include "file_reader.h"

#define WINDOW_WIDTH 1220
#define WINDOW_HEIGHT 860
#define NODE_RADIUS 24

typedef struct {
    float x;
    float y;
} Point;

typedef struct {
    int* nodes;
    int length;
} Path;

// Function to safely free the path memory
static void free_path(Path* path) {
    if (path == NULL) return;
    free(path->nodes);
    path->nodes = NULL;
    path->length = 0;
}

// Function to draw the original edge weight style
static void draw_edge_weight(int x, int y, int weight) {
    const int badge_r = 18;
    const char* text = TextFormat("%d", weight);
    int text_w = MeasureText(text, 22);
    int tx = x - text_w / 2;
    int ty = y - 11;

    // Draw the badge background and border
    DrawCircle(x, y, badge_r + 2, (Color){8, 12, 34, 255});
    DrawCircle(x, y, badge_r, (Color){22, 34, 74, 255});
    DrawCircleLines(x, y, (float)badge_r, (Color){255, 232, 145, 255});

    // Draw fake bold text by rendering it multiple times with slight offsets
    DrawText(text, tx - 1, ty, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx + 1, ty, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx, ty - 1, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx, ty + 1, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx, ty, 22, (Color){255, 252, 210, 255});
}

// Function to draw a prominent directional arrow on an edge
static void draw_arrow(Vector2 from, Vector2 to, Color color) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    
    if (len > 0.001f) {
        float ux = dx / len;
        float uy = dy / len;
        
        // Position the arrow at 75% of the edge length to prevent overlap with the destination node
        Vector2 tip = { from.x + dx * 0.75f, from.y + dy * 0.75f };
        
        float arrow_len = 22.0f;   // Length of the arrowhead
        float arrow_width = 14.0f; // Width of the arrowhead
        
        // Calculate the two base vertices of the triangle representing the arrowhead
        Vector2 left = { tip.x - ux * arrow_len + uy * arrow_width, tip.y - uy * arrow_len - ux * arrow_width };
        Vector2 right = { tip.x - ux * arrow_len - uy * arrow_width, tip.y - uy * arrow_len + ux * arrow_width };
        
        // Draw the triangle twice with different vertex orders to ensure it renders correctly regardless of culling
        DrawTriangle(tip, left, right, color);
        DrawTriangle(tip, right, left, color);
    }
}

// Function to draw the original "ghost" styled nodes
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

    // Draw the top semi-circle and the rectangular body
    DrawCircleV((Vector2){p.x, p.y - size * 0.38f}, top_r, body_color);
    DrawRectangleRounded((Rectangle){rect_x, rect_y, body_w, body_h}, 0.05f, 8, body_color);

    // Draw the three "feet" of the ghost
    DrawCircleV((Vector2){rect_x + body_w * 0.20f, rect_y + body_h}, size * 0.19f, body_color);
    DrawCircleV((Vector2){rect_x + body_w * 0.50f, rect_y + body_h}, size * 0.19f, body_color);
    DrawCircleV((Vector2){rect_x + body_w * 0.80f, rect_y + body_h}, size * 0.19f, body_color);

    // Draw the eyes (white background and blue pupils)
    DrawCircleV((Vector2){p.x - size * 0.20f, p.y - size * 0.50f}, eye_r, RAYWHITE);
    DrawCircleV((Vector2){p.x + size * 0.20f, p.y - size * 0.50f}, eye_r, RAYWHITE);
    DrawCircleV((Vector2){p.x - size * 0.16f, p.y - size * 0.47f}, pupil_r, BLUE);
    DrawCircleV((Vector2){p.x + size * 0.24f, p.y - size * 0.47f}, pupil_r, BLUE);

    // Draw the node ID on the ghost's body
    DrawText(id_text, (int)(p.x - id_w / 2.0f), (int)(p.y + size * 0.17f), font_size, (Color){15, 22, 45, 255});
}

// Function to arrange the nodes in a circular layout
static Point* build_layout(int n) {
    Point* positions = (Point*)malloc((size_t)n * sizeof(Point));
    float cx = WINDOW_WIDTH / 2.0f;
    float cy = WINDOW_HEIGHT / 2.0f;
    float r = (WINDOW_HEIGHT < WINDOW_WIDTH ? WINDOW_HEIGHT : WINDOW_WIDTH) * 0.40f;

    for (int i = 0; i < n; i++) {
        // Calculate the angle for each node, starting from the top (-PI/2)
        float angle = (2.0f * (float)PI * (float)i / (float)n) - (float)PI / 2.0f;
        positions[i].x = cx + r * cosf(angle);
        positions[i].y = cy + r * sinf(angle);
    }
    return positions;
}

// Function to convert the DijkstraResult into an ordered Path structure
static Path build_path(const DijkstraResult* result) {
    Path path = {NULL, 0};
    // Return an empty path if there's no result or no valid route found
    if (result == NULL || result->dist[result->dest] == INT_MAX) return path;

    int current = result->dest;
    int length = 0;
    // Temporary array to trace the path backwards
    int* reversed = (int*)malloc((size_t)result->num_vertices * sizeof(int));

    while (current != -1) {
        reversed[length++] = current;
        if (current == result->src) break; // Stop when the source node is reached
        current = result->prev[current];
    }

    // Reverse the traced path to represent it from source to destination
    if (length > 0 && reversed[length - 1] == result->src) {
        path.nodes = (int*)malloc((size_t)length * sizeof(int));
        path.length = length;
        for (int i = 0; i < length; i++) {
            path.nodes[i] = reversed[length - 1 - i];
        }
    }
    free(reversed);
    return path;
}

// Main rendering function to draw the static graph and the shortest path
static void render_scene(const Graph* graph, const Point* positions, const Path* shortest_path) {
    // Original color palette for the ghost nodes
    static const Color ghost_palette[] = {
        (Color){255, 60, 70, 255}, (Color){0, 210, 255, 255},
        (Color){255, 190, 50, 255}, (Color){255, 150, 220, 255},
        (Color){120, 255, 145, 255}, (Color){170, 130, 255, 255}
    };
    int palette_count = 6;

    BeginDrawing();
    ClearBackground((Color){9, 13, 30, 255});

    // 1. Draw base edges and their weights (No arrows drawn here)
    for (int i = 0; i < graph->num_vertices; i++) {
        Edge* edge = graph->adj_lists[i];
        while (edge != NULL) {
            Vector2 a = {positions[i].x, positions[i].y};
            Vector2 b = {positions[edge->dest].x, positions[edge->dest].y};
            
            Color edge_color = (Color){90, 110, 170, 255};
            DrawLineEx(a, b, 2.0f, edge_color);
            
            // Calculate the position for the edge weight badge
            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float len = sqrtf(dx * dx + dy * dy);
            float nx = 0.0f, ny = 1.0f;
            if (len > 0.001f) { 
                nx = -dy / len; 
                ny = dx / len; 
            }
            float side = ((i + edge->dest) % 2 == 0) ? 1.0f : -1.0f;
            
            // Offset the badge slightly to avoid overlapping parallel edges going opposite ways
            float wx = (a.x + b.x) * 0.5f + nx * 22.0f * side;
            float wy = (a.y + b.y) * 0.5f + ny * 22.0f * side;
            draw_edge_weight((int)wx, (int)wy, edge->weight);

            edge = edge->next;
        }
    }

    // 2. Draw the shortest path on top of the base edges (Highlighted in yellow with arrows)
    if (shortest_path->length >= 2) {
        for (int i = 0; i < shortest_path->length - 1; i++) {
            int a = shortest_path->nodes[i];
            int b = shortest_path->nodes[i + 1];
            Vector2 p1 = {positions[a].x, positions[a].y};
            Vector2 p2 = {positions[b].x, positions[b].y};
            
            // Draw a thicker yellow line for the path segment
            DrawLineEx(p1, p2, 4.0f, YELLOW);
            // Draw the directional arrow indicating the path flow
            draw_arrow(p1, p2, YELLOW);
        }
    }

    // 3. Draw the nodes (ghosts) last so they appear on top of all lines and arrows
    for (int i = 0; i < graph->num_vertices; i++) {
        Color node_color = ghost_palette[i % palette_count];
        draw_ghost_node((Vector2){positions[i].x, positions[i].y}, (float)NODE_RADIUS * 1.25f, node_color, i);
    }

    EndDrawing();
}

int main(void) {
    int start, end;
    // Load the graph structure from the input file
    Graph* graph = read_graph_from_file("input.txt", &start, &end);
    if (graph == NULL) return 1;

    // Calculate the shortest path using Dijkstra's algorithm
    DijkstraResult* result = dijkstra(graph, start, end);
    // Convert the result structure into an array-based path
    Path shortest_path = build_path(result);

    // Calculate the physical screen coordinates for each node
    Point* positions = build_layout(graph->num_vertices);

    // Initialize the raylib window
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Milestone 2 - Static Shortest Path GUI");
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose()) {
        render_scene(graph, positions, &shortest_path);
    }

    // Close the window and OpenGL context
    CloseWindow();
    
    // Free all dynamically allocated memory
    free(positions);
    free_path(&shortest_path);
    free_dijkstra_result(result);
    free_graph(graph);
    
    return 0;
}