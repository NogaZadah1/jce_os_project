#include "gui.h"

#include <math.h>
#include <stdlib.h>

static void draw_edge_weight(int x, int y, int weight) {
    const int badge_r = 18;
    const char* text = TextFormat("%d", weight);
    int text_w = MeasureText(text, 22);
    int tx = x - text_w / 2;
    int ty = y - 11;

    DrawCircle(x, y, badge_r + 2, (Color){8, 12, 34, 255});
    DrawCircle(x, y, badge_r, (Color){22, 34, 74, 255});
    DrawCircleLines(x, y, (float)badge_r, (Color){255, 232, 145, 255});

    DrawText(text, tx - 1, ty, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx + 1, ty, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx, ty - 1, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx, ty + 1, 22, (Color){14, 18, 45, 255});
    DrawText(text, tx, ty, 22, (Color){255, 252, 210, 255});
}

static void draw_arrow(Vector2 from, Vector2 to, Color color) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);

    if (len > 0.001f) {
        float ux = dx / len;
        float uy = dy / len;

        Vector2 tip = { from.x + dx * 0.75f, from.y + dy * 0.75f };

        float arrow_len = 22.0f;
        float arrow_width = 14.0f;

        Vector2 left = {
            tip.x - ux * arrow_len + uy * arrow_width,
            tip.y - uy * arrow_len - ux * arrow_width
        };

        Vector2 right = {
            tip.x - ux * arrow_len - uy * arrow_width,
            tip.y - uy * arrow_len + ux * arrow_width
        };

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
    int id_w;

    if (font_size < 18) font_size = 18;
    if (font_size > 26) font_size = 26;

    id_w = MeasureText(id_text, font_size);

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

    DrawText(
        id_text,
        (int)(p.x - id_w / 2.0f),
        (int)(p.y + size * 0.17f),
        font_size,
        (Color){15, 22, 45, 255}
    );
}

static void draw_pacman(Vector2 p, float radius, float angle_deg, Color body_color, Color outline_color) {
    float mouth = 36.0f;
    float eye_angle = angle_deg - 40.0f;

    Vector2 eye_pos = {
        p.x + cosf(eye_angle * DEG2RAD) * radius * 0.35f,
        p.y + sinf(eye_angle * DEG2RAD) * radius * 0.35f
    };

    DrawCircleSector(p, radius, angle_deg + mouth, angle_deg + (360.0f - mouth), 48, body_color);
    DrawCircleSectorLines(p, radius, angle_deg + mouth, angle_deg + (360.0f - mouth), 48, outline_color);
    DrawCircleV(eye_pos, radius * 0.12f, BLACK);
}

Point* build_layout(int n) {
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

int get_edge_weight(const Graph* graph, int src, int dest) {
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
) 
{

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
            draw_arrow(p1, p2, path_color);
        }
    }

    for (i = 0; i < graph->num_vertices; i++) {
        Color node_color = ghost_palette[i % palette_count];

        draw_ghost_node(
            (Vector2){positions[i].x, positions[i].y},
            (float)NODE_RADIUS * 1.25f,
            node_color,
            i
        );
    }

    (void)food_alive;

    Rectangle play_button = {30, 30, 130, 45};
    const char* button_text = is_playing ? "Stop" : "Play";

    DrawRectangleRounded(play_button, 0.25f, 8, (Color){30, 90, 150, 255});
    DrawRectangleRoundedLines(play_button, 0.25f, 8, (Color){255, 255, 255, 255});
    DrawText(button_text, (int)(play_button.x + 35), (int)(play_button.y + 12), 22, RAYWHITE);

    if (arrived == 1) {
        DrawText(
            "Arrived at destination!",
            WINDOW_WIDTH / 2 - 160,
            30,
            26,
            RAYWHITE
        );
    }

    if (traveler_positions != NULL && travelers != NULL && traveler_count > 0) {
        for (i = 0; i < traveler_count; i++) {
            Color traveler_color = ghost_palette[i % palette_count];

            draw_pacman(
                (Vector2){traveler_positions[i].x, traveler_positions[i].y},
                (float)PACMAN_RADIUS + 1.5f,
                0.0f,
                traveler_color,
                RAYWHITE
            );
        }
    } else {
        draw_pacman(
            (Vector2){pacman_x, pacman_y},
            (float)PACMAN_RADIUS + 1.5f,
            pacman_angle_deg,
            YELLOW,
            GOLD
        );
    }
    EndDrawing();
}