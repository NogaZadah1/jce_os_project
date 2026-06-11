#include "m5_gui_adapter.h"

#include <stdio.h>
#include <stdlib.h>

#define EDGE_STEP_SECONDS 0.300
#define NODE_WAIT_SECONDS 1.000

typedef struct {
    int* queue;
    int queue_capacity;
    int queue_head;
    int queue_tail;
    int queue_count;

    int current_node;
    int target_node;

    int is_moving;
    int is_waiting;

    int is_blocked_waiting;
    int waiting_node;

    Point start_position;
    Point target_position;

    double move_start_time;
    double move_duration;
    double wait_until_time;
} M5TravelerVisualState;

static M5TravelerVisualState* g_visual_states = NULL;
static int g_traveler_count = 0;
static int g_m5_is_playing = 1;
static const Graph* g_graph = NULL;

static int queue_push(M5TravelerVisualState* state, int node) {
    if (state == NULL || state->queue == NULL) {
        return 0;
    }

    if (state->queue_count >= state->queue_capacity) {
        return 0;
    }

    state->queue[state->queue_tail] = node;
    state->queue_tail = (state->queue_tail + 1) % state->queue_capacity;
    state->queue_count++;

    return 1;
}

static int queue_pop(M5TravelerVisualState* state, int* node) {
    if (state == NULL || state->queue == NULL || node == NULL) {
        return 0;
    }

    if (state->queue_count <= 0) {
        return 0;
    }

    *node = state->queue[state->queue_head];
    state->queue_head = (state->queue_head + 1) % state->queue_capacity;
    state->queue_count--;

    return 1;
}

/*
 * Handles the Play/Stop button for milestone 5/6 animation.
 * The button itself is drawn by render_scene.
 * This adapter only controls whether traveler animations advance.
 */
static void update_play_button_state(void) {
    Rectangle play_button = {30, 30, 130, 45};

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(GetMousePosition(), play_button)) {
        g_m5_is_playing = !g_m5_is_playing;
    }
}

static int get_edge_weight_for_animation(
    const Graph* graph,
    int src,
    int dest
) {
    Edge* current;

    if (graph == NULL) {
        return 1;
    }

    if (src < 0 || src >= graph->num_vertices) {
        return 1;
    }

    current = graph->adj_lists[src];

    while (current != NULL) {
        if (current->dest == dest) {
            if (current->weight > 0) {
                return current->weight;
            }

            return 1;
        }

        current = current->next;
    }

    return 1;
}

static Point interpolate_point(Point start, Point end, float t) {
    Point result;

    result.x = start.x + (end.x - start.x) * t;
    result.y = start.y + (end.y - start.y) * t;

    return result;
}

static void start_next_move_if_needed(
    M5TravelerVisualState* state,
    const Point* node_positions,
    Point* traveler_position
) {
    int next_node;
    int weight;

    if (state == NULL || node_positions == NULL || traveler_position == NULL) {
        return;
    }

    if (state->is_blocked_waiting || state->is_moving || state->is_waiting) {
        return;
    }

    if (!queue_pop(state, &next_node)) {
        return;
    }

    state->target_node = next_node;
    state->start_position = *traveler_position;
    state->target_position = node_positions[next_node];

    weight = get_edge_weight_for_animation(
        g_graph,
        state->current_node,
        state->target_node
    );

    if (weight <= 0) {
        weight = 1;
    }

    /*
     * Milestone 3 timing rule:
     * edge with weight W takes W * 300 ms.
     *
     * We animate smoothly during that total duration.
     */
    state->move_duration = (double)weight * EDGE_STEP_SECONDS;
    state->move_start_time = GetTime();
    state->is_moving = 1;
}

static void update_single_traveler_animation(
    M5TravelerVisualState* state,
    const Point* node_positions,
    Point* traveler_position
) {
    double now;
    double elapsed;
    float t;

    if (state == NULL || node_positions == NULL || traveler_position == NULL) {
        return;
    }

    now = GetTime();

    if (state->is_waiting) {
        if (now >= state->wait_until_time) {
            state->is_waiting = 0;
        } else {
            return;
        }
    }

    start_next_move_if_needed(state, node_positions, traveler_position);

    if (!state->is_moving) {
        return;
    }

    elapsed = now - state->move_start_time;

    if (state->move_duration <= 0.0) {
        state->move_duration = EDGE_STEP_SECONDS;
    }

    if (elapsed >= state->move_duration) {
        *traveler_position = state->target_position;

        state->is_moving = 0;
        state->current_node = state->target_node;
        state->target_node = -1;

        /*
         * Milestone 3 rule:
         * wait one second at middle nodes only.
         *
         * If the queue is empty, this is the final destination,
         * so there is no extra waiting.
         */
        if (state->queue_count > 0) {
            state->is_waiting = 1;
            state->wait_until_time = now + NODE_WAIT_SECONDS;
        }

        return;
    }

    t = (float)(elapsed / state->move_duration);

    if (t < 0.0f) {
        t = 0.0f;
    }

    if (t > 1.0f) {
        t = 1.0f;
    }

    *traveler_position = interpolate_point(
        state->start_position,
        state->target_position,
        t
    );
}

static void update_all_traveler_animations(
    const Point* node_positions,
    Point* traveler_positions
) {
    int i;

    if (g_visual_states == NULL || node_positions == NULL || traveler_positions == NULL) {
        return;
    }

    for (i = 0; i < g_traveler_count; i++) {
        update_single_traveler_animation(
            &g_visual_states[i],
            node_positions,
            &traveler_positions[i]
        );
    }
}

int m5_gui_init_state(
    const Graph* graph,
    const Traveler* travelers,
    int traveler_count,
    Point** node_positions,
    Point** traveler_positions
) {
    int i;

    if (graph == NULL || travelers == NULL ||
        node_positions == NULL || traveler_positions == NULL ||
        traveler_count <= 0) {
        return 0;
    }

    if (!IsWindowReady()) {
        SetTraceLogLevel(LOG_WARNING);

        InitWindow(
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            "Milestone 5 - IPC Traffic Simulation"
        );
        SetTargetFPS(60);
    }

    g_graph = graph;
    g_traveler_count = traveler_count;

    *node_positions = build_layout(graph->num_vertices);
    if (*node_positions == NULL) {
        fprintf(stderr, "Error: failed to build GUI node layout\n");
        return 0;
    }

    *traveler_positions = malloc((size_t)traveler_count * sizeof(Point));
    if (*traveler_positions == NULL) {
        fprintf(stderr, "Error: failed to allocate traveler positions\n");
        free(*node_positions);
        *node_positions = NULL;
        return 0;
    }

    g_visual_states = malloc((size_t)traveler_count * sizeof(M5TravelerVisualState));
    if (g_visual_states == NULL) {
        fprintf(stderr, "Error: failed to allocate visual states\n");
        free(*node_positions);
        free(*traveler_positions);
        *node_positions = NULL;
        *traveler_positions = NULL;
        return 0;
    }

    for (i = 0; i < traveler_count; i++) {
        int source = travelers[i].source;

        if (source < 0 || source >= graph->num_vertices) {
            fprintf(stderr, "Error: invalid traveler source node %d\n", source);

            m5_gui_free_state(*node_positions, *traveler_positions);

            *node_positions = NULL;
            *traveler_positions = NULL;

            return 0;
        }

        (*traveler_positions)[i] = (*node_positions)[source];

        /*
         * The queue stores the next nodes received from IPC messages.
         * Capacity graph->num_vertices + 2 is enough for a simple path.
         */
        g_visual_states[i].queue_capacity = graph->num_vertices + 2;
        g_visual_states[i].queue = malloc(
            (size_t)g_visual_states[i].queue_capacity * sizeof(int)
        );

        if (g_visual_states[i].queue == NULL) {
            fprintf(stderr, "Error: failed to allocate traveler queue\n");

            m5_gui_free_state(*node_positions, *traveler_positions);

            *node_positions = NULL;
            *traveler_positions = NULL;

            return 0;
        }

        g_visual_states[i].queue_head = 0;
        g_visual_states[i].queue_tail = 0;
        g_visual_states[i].queue_count = 0;

        g_visual_states[i].current_node = source;
        g_visual_states[i].target_node = -1;

        g_visual_states[i].is_moving = 0;
        g_visual_states[i].is_waiting = 0;
        g_visual_states[i].is_blocked_waiting = 0;
        g_visual_states[i].waiting_node = -1;

        g_visual_states[i].start_position = (*traveler_positions)[i];
        g_visual_states[i].target_position = (*traveler_positions)[i];

        g_visual_states[i].move_start_time = 0.0;
        g_visual_states[i].move_duration = 0.0;
        g_visual_states[i].wait_until_time = 0.0;
    }

    return 1;
}

void m5_gui_apply_arrival(
    const IpcMessage* message,
    const Point* node_positions,
    Point* traveler_positions
) {
    M5TravelerVisualState* state;

    if (message == NULL) {
        return;
    }

    if (message->type != IPC_MSG_ARRIVED) {
        return;
    }

    if (g_visual_states == NULL) {
        return;
    }

    if (message->traveler_id < 0 || message->traveler_id >= g_traveler_count) {
        return;
    }

    state = &g_visual_states[message->traveler_id];

    if (state->is_blocked_waiting &&
        node_positions != NULL &&
        traveler_positions != NULL &&
        message->current_node >= 0) {
        traveler_positions[message->traveler_id] = node_positions[message->current_node];
        state->current_node = message->current_node;
        state->target_node = -1;
        state->is_moving = 0;
        state->is_waiting = 0;
        state->is_blocked_waiting = 0;
        state->waiting_node = -1;
    }

    /*
     * The child sends:
     * current_node = node it reached
     * next_node    = next node in the route, or IPC_DESTINATION_NODE
     *
     * For the GUI, we queue only the next node.
     * The adapter will animate the traveler from its current visual node
     * to that queued node.
     */
    if (message->next_node != IPC_DESTINATION_NODE) {
        queue_push(state, message->next_node);
    }
}


void m5_gui_apply_waiting(
    const IpcMessage* message,
    const Point* node_positions,
    Point* traveler_positions
) {
    M5TravelerVisualState* state;
    float offset_x;
    float offset_y;
    int traveler_id;
    int waiting_node;

    if (message == NULL || node_positions == NULL || traveler_positions == NULL) {
        return;
    }

    if (message->type != IPC_MSG_WAITING) {
        return;
    }

    if (g_visual_states == NULL) {
        return;
    }

    traveler_id = message->traveler_id;
    waiting_node = message->current_node;

    if (traveler_id < 0 || traveler_id >= g_traveler_count) {
        return;
    }

    if (waiting_node < 0) {
        return;
    }

    state = &g_visual_states[traveler_id];

    /*
     * Show the traveler slightly outside the occupied node.
     * Different offsets make several waiting travelers visible at once.
     */
    offset_x = 34.0f + (float)(traveler_id % 3) * 14.0f;
    offset_y = 34.0f + (float)(traveler_id / 3) * 14.0f;

    traveler_positions[traveler_id].x = node_positions[waiting_node].x + offset_x;
    traveler_positions[traveler_id].y = node_positions[waiting_node].y + offset_y;

    state->is_blocked_waiting = 1;
    state->waiting_node = waiting_node;
    state->is_moving = 0;
    state->is_waiting = 0;
    state->target_node = waiting_node;
}

void m5_gui_render_frame(
    const Graph* graph,
    const Point* node_positions,
    const Traveler* travelers,
    int traveler_count,
    const Point* traveler_positions
) {
    Path empty_path;
    int* food_alive;
    int* traveler_waiting_flags;
    int i;
    Point* mutable_traveler_positions;

    if (graph == NULL || node_positions == NULL ||
        travelers == NULL || traveler_positions == NULL) {
        return;
    }

    if (!IsWindowReady()) {
        return;
    }
        update_play_button_state();

    /*
     * The function receives const traveler_positions because render_scene
     * should not modify it. However, this adapter owns the array allocated
     * in m5_gui_init_state, so it advances the animation here.
     */
    mutable_traveler_positions = (Point*)traveler_positions;

        if (g_m5_is_playing) {
        update_all_traveler_animations(
            node_positions,
            mutable_traveler_positions
        );
    }

    empty_path.nodes = NULL;
    empty_path.length = 0;

    food_alive = malloc((size_t)graph->num_vertices * sizeof(int));
    if (food_alive == NULL) {
        fprintf(stderr, "Error: failed to allocate food state\n");
        return;
    }

    traveler_waiting_flags = malloc((size_t)traveler_count * sizeof(int));
    if (traveler_waiting_flags == NULL) {
        fprintf(stderr, "Error: failed to allocate waiting state\n");
        free(food_alive);
        return;
    }

    for (i = 0; i < graph->num_vertices; i++) {
        food_alive[i] = 1;
    }

    for (i = 0; i < traveler_count; i++) {
        traveler_waiting_flags[i] = 0;
        if (g_visual_states != NULL && i < g_traveler_count) {
            traveler_waiting_flags[i] = g_visual_states[i].is_blocked_waiting;
        }
    }

    render_scene(
        graph,
        node_positions,
        &empty_path,
        food_alive,
        0.0f,
        0.0f,
        0.0f,
        g_m5_is_playing,
        0,
        travelers,
        traveler_count,
        mutable_traveler_positions,
        traveler_waiting_flags
    );

    free(traveler_waiting_flags);
    free(food_alive);
}

void m5_gui_free_state(
    Point* node_positions,
    Point* traveler_positions
) {
    int i;

    if (g_visual_states != NULL) {
        for (i = 0; i < g_traveler_count; i++) {
            free(g_visual_states[i].queue);
            g_visual_states[i].queue = NULL;
        }

        free(g_visual_states);
        g_visual_states = NULL;
    }

    g_traveler_count = 0;
    g_m5_is_playing = 1;
    g_graph = NULL;

    free(node_positions);
    free(traveler_positions);

    if (IsWindowReady()) {
        CloseWindow();
    }
}