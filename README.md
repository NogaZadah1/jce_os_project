# OS Project - Graph Traffic Simulation

This project implements a directed weighted graph traffic simulation in C for Linux.

The project starts with a command-line implementation of Dijkstra's shortest path algorithm and gradually extends it into a Raylib-based graphical simulation with animation, multiple traveler processes, and IPC communication between child processes and a parent process.

---

## Main Features

- Directed weighted graph represented with adjacency lists
- Graph and traveler input parsing from text files
- Dijkstra shortest path calculation
- Static graph visualization with Raylib
- Animated movement along shortest paths
- Multiple travelers moving simultaneously
- Child process creation using `fork()`
- IPC communication using pipes
- Parent-controlled GUI updates and terminal logging

---

## Build and Run

Clean compiled files:

```bash
make clean
````

### Milestone 1 - Dijkstra CLI

```bash
make milestone1
./dijkstra <file_name>
```

### Milestone 2 - Static Graph GUI

```bash
make milestone2
./sim <file_name>
```

### Milestone 3 - Animated Single Traveler

```bash
make milestone3
./sim <file_name>
```

### Milestone 4 - Multiple Travelers

```bash
make milestone4
./sim <file_name>
```

Example:

```bash
./sim input_m4.txt
```

### Milestone 5 - IPC-Based Travelers

```bash
make milestone5
./sim <file_name>
```

Example:

```bash
./sim input_milestone5.txt
```

---

## Milestone Summary

### Milestone 1 - Dijkstra CLI

Milestone 1 implements the basic command-line shortest path program.

The program reads a directed weighted graph from an input file, receives the source and destination from the same file, runs Dijkstra's algorithm, and prints the shortest path and its total cost.

If no path exists, the program prints:

```text
No path found
```

### Milestone 2 - Static Graph GUI

Milestone 2 adds a Raylib-based graphical display.

The program draws the graph on screen, including nodes, directed edges, edge weights, and the shortest path calculated by Dijkstra. This stage focuses on visualizing the graph and the selected shortest path without animation.

### Milestone 3 - Animated Single Traveler

Milestone 3 adds animation for a single traveler moving along the shortest path.

The traveler moves from node to node according to the path calculated by Dijkstra. Edge travel time is based on the edge weight, and the traveler pauses at intermediate nodes before continuing.

### Milestone 4 - Multiple Travelers and Child Processes

Milestone 4 extends the simulation to multiple travelers.

The parent process reads the extended input file, calculates the shortest path for each traveler, creates one child process per traveler using `fork()`, and manages the Raylib GUI.

In this milestone, the parent process still performs the route calculations and controls the visual simulation. The child processes only stay alive during the travel time and print a start message when created. When a traveler reaches its destination, the parent terminates the matching child process and waits for all children before exiting.

### Milestone 5 - IPC-Based Autonomous Travelers

Milestone 5 changes the architecture so that each child process becomes responsible for its own route calculation.

Each child process receives the graph, source, destination, write side of a pipe, and traveler ID. The child runs Dijkstra independently, builds its own path, and sends progress messages to the parent whenever it reaches a node.

The parent process receives IPC messages from all children, prints the required log messages to the terminal, and updates the GUI according to the reported traveler positions.

In this milestone, all terminal output is produced by the parent process only.

---

## Input Formats

### Basic Input Format

Used by milestones 1-3:

```text
N M
src dst weight
src dst weight
...
source destination
```

Example:

```text
6 8
0 1 4
0 2 2
1 3 5
2 1 1
2 3 8
3 4 2
4 5 3
2 5 10
0 5
```

Example output:

```text
0 -> 2 -> 1 -> 3 -> 4 -> 5
12
```

### Extended Travelers Input Format

Used by milestones 4-5:

```text
N M
src dst weight
src dst weight
...
traveler_count
source destination
source destination
...
```

Example:

```text
5 7
0 1 4
0 2 2
1 3 5
2 1 1
2 3 8
3 4 2
1 4 6
2
0 4
2 3
```

---

## IPC Design - Milestone 5

Milestone 5 uses pipes for IPC.

Each child process has a dedicated pipe used to send messages to the parent process:

```text
child process -> parent process
```

Pipes were chosen because the communication pattern is one-way: each child only reports its progress, while the parent is responsible for logging, GUI updates, and process management.

This design keeps the child processes independent while still allowing the parent process to coordinate the global simulation state.

---

## IPC Message Format

Defined in `ipc.h`:

```c
typedef enum {
    IPC_MSG_ARRIVED,
    IPC_MSG_FINISHED,
    IPC_MSG_ERROR
} IpcMessageType;

typedef struct {
    IpcMessageType type;
    pid_t pid;
    int traveler_id;
    int current_node;
    int next_node;
} IpcMessage;
```

Message fields:

| Field          | Meaning                                      |
| -------------- | -------------------------------------------- |
| `type`         | Message type: arrived, finished, or error    |
| `pid`          | Child process ID                             |
| `traveler_id`  | Traveler index in the travelers array        |
| `current_node` | Node reached by the traveler                 |
| `next_node`    | Next node in the path, or destination marker |

---

## Milestone 5 Log Example

```text
[PID=1021] arrived at node 0 | next node: 2
[PID=1022] arrived at node 2 | next node: 1
[PID=1021] arrived at node 2 | next node: 1
[PID=1022] arrived at node 1 | next node: 3
[PID=1021] arrived at node 1 | next node: 4
[PID=1022] arrived at node 3 | DESTINATION
[PID=1021] arrived at node 4 | DESTINATION
[PID=1022] finished
[PID=1021] finished
```

---

## Files

| File                                         | Description                                                |
| -------------------------------------------- | ---------------------------------------------------------- |
| `graph.c`, `graph.h`                         | Graph representation using adjacency lists                 |
| `dijkstra.c`, `dijkstra.h`                   | Dijkstra shortest path algorithm                           |
| `file_reader.c`, `file_reader.h`             | Input parsing for graph and traveler files                 |
| `gui.c`, `gui.h`                             | Shared Raylib drawing and rendering functions              |
| `traveler.c`, `traveler.h`                   | Traveler data, child process logic, and traveler utilities |
| `ipc.c`, `ipc.h`                             | IPC message structure and pipe send/read helpers           |
| `parent_controller.c`, `parent_controller.h` | Parent process logic for milestone 5                       |
| `m5_gui_adapter.c`, `m5_gui_adapter.h`       | Adapter between IPC state updates and GUI rendering        |
| `main_dijkstra.c`                            | Entry point for milestone 1                                |
| `main_static.c`                              | Entry point for milestone 2                                |
| `main_sim.c`                                 | Entry point for milestone 3                                |
| `main_m4.c`                                  | Entry point for milestone 4                                |
| `main_m5.c`                                  | Entry point for milestone 5                                |
| `input.txt`                                  | Example input for milestones 1-3                           |
| `input_m4.txt`                               | Example input for milestone 4                              |
| `input_milestone5.txt`                       | Example input for milestone 5                              |
| `Makefile`                                   | Build targets for all milestones                           |

---

## Notes

* Written in C
* Designed and tested on Linux
* GUI implemented with Raylib
* Shortest paths are calculated with Dijkstra's algorithm
* Milestone 4 uses multiple child processes
* Milestone 5 uses pipes for IPC

````
