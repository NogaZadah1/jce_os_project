#ifndef M5_GUI_ADAPTER_H
#define M5_GUI_ADAPTER_H

#include "graph.h"
#include "traveler.h"
#include "gui.h"
#include "ipc.h"

/*
 * Initializes the GUI state for Milestone 5.
 *
 * Creates:
 * 1. node_positions - positions of graph nodes on the screen
 * 2. traveler_positions - current visual position of each traveler
 *
 * Each traveler starts at its source node.
 *
 * Returns 1 on success, 0 on failure.
 */
int m5_gui_init_state(
    const Graph* graph,
    const Traveler* travelers,
    int traveler_count,
    Point** node_positions,
    Point** traveler_positions
);

/*
 * Receives an IPC ARRIVED message from the parent controller
 * and converts it into a visual movement request.
 *
 * The child reports:
 * current_node = the node it reached
 * next_node    = the next node in the path, or IPC_DESTINATION_NODE
 *
 * The adapter queues the next node so the GUI can animate movement
 * using the Milestone 3 timing rules.
 */
void m5_gui_apply_arrival(
    const IpcMessage* message,
    const Point* node_positions,
    Point* traveler_positions
);

/*
 * Receives an IPC WAITING message and marks the traveler as visually waiting
 * outside the requested node.
 */
void m5_gui_apply_waiting(
    const IpcMessage* message,
    const Point* node_positions,
    Point* traveler_positions
);

/*
 * Renders one frame of the Milestone 5 simulation.
 *
 * This function also advances the visual animation of each traveler:
 * - Edge weight W means W jumps.
 * - Each jump takes 300 ms.
 * - Middle nodes have a 1 second pause.
 */
void m5_gui_render_frame(
    const Graph* graph,
    const Point* node_positions,
    const Traveler* travelers,
    int traveler_count,
    const Point* traveler_positions
);

/*
 * Frees all GUI state allocated by m5_gui_init_state.
 */
void m5_gui_free_state(
    Point* node_positions,
    Point* traveler_positions
);

#endif