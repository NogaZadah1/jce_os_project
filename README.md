# OS Project - Graph Traffic Simulation

This project implements a directed weighted graph traffic simulation in C on Linux.

The project includes graph representation, Dijkstra shortest path calculation, graph visualization with Raylib, animated movement on the graph, multiple traveler processes, and IPC communication between child processes and the parent process.

---

## Main Features

* Directed weighted graph using adjacency lists
* Reading graph and travelers from input files
* Dijkstra shortest path algorithm
* Static graph visualization with Raylib
* Animated movement based on edge weights
* Multiple travelers moving on the graph
* Child processes created with `fork()`
* IPC communication using pipes
* Parent-only logging in Milestone 5

---

## Files

| File                                         | Description                            |
| -------------------------------------------- | -------------------------------------- |
| `graph.c`, `graph.h`                         | Graph representation                   |
| `dijkstra.c`, `dijkstra.h`                   | Dijkstra shortest path algorithm       |
| `file_reader.c`, `file_reader.h`             | Reading graph and traveler input files |
| `gui.c`, `gui.h`                             | Shared Raylib GUI rendering            |
| `traveler.c`, `traveler.h`                   | Traveler and child process logic       |
| `ipc.c`, `ipc.h`                             | IPC message structure and pipe helpers |
| `parent_controller.c`, `parent_controller.h` | Parent process logic for Milestone 5   |
| `m5_gui_adapter.c`, `m5_gui_adapter.h`       | GUI adapter for IPC-based updates      |
| `main_dijkstra.c`                            | Milestone 1 main file                  |
| `main_static.c`                              | Milestone 2 main file                  |
| `main_sim.c`                                 | Milestone 3 main file                  |
| `main_m4.c`                                  | Milestone 4 main file                  |
| `main_m5.c`                                  | Milestone 5 main file                  |
| `input.txt`                                  | Basic input file                       |
| `input_m4.txt`                               | Multiple travelers input file          |
| `input_milestone5.txt`                       | Milestone 5 input file                 |
| `Makefile`                                   | Build targets                          |

---

## Build and Run

Clean compiled files:

```bash
make clean
```

### Milestone 1

```bash
make milestone1
./dijkstra input.txt
```

### Milestone 2

```bash
make milestone2
./sim input.txt
```

### Milestone 3

```bash
make milestone3
./sim input.txt
```

### Milestone 4

```bash
make milestone4
./sim input_m4.txt
```

### Milestone 5

```bash
make milestone5
./sim input_milestone5.txt
```

---

## Basic Input Format

Used in Milestones 1-3:

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

Expected output:

```text
0 -> 2 -> 1 -> 3 -> 4 -> 5
12
```

If no path exists:

```text
No path found
```

---

## Extended Input Format

Used in Milestones 4-5:

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

## Milestone Summary

### Milestone 1 - Dijkstra CLI

Reads a directed weighted graph from a file, runs Dijkstra from source to destination, and prints the shortest path and total weight.

### Milestone 2 - Static GUI

Displays the graph in a Raylib window, including nodes, directed edges, and edge weights.

### Milestone 3 - Animation

Animates movement on the graph according to the calculated path.
Edge travel time depends on edge weight: an edge with weight `W` takes `W * 300ms`.
The traveler waits one second at intermediate nodes.

### Milestone 4 - Multiple Travelers

Adds multiple travelers.
The parent process creates child processes with `fork()`, manages the GUI, and displays several travelers moving at the same time.

### Milestone 5 - IPC

Each child process calculates its own Dijkstra path and reports its progress to the parent process using IPC.

The parent process receives messages, prints the required log, and updates the GUI.

Children do not print to the terminal in Milestone 5.

---

## IPC Design

For Milestone 5, we chose to use pipes.

Each child process has its own pipe to send messages to the parent:

```text
child process -> parent process
```

Pipes were chosen because the communication is one-way: each child only needs to report its progress, while the parent handles logging and GUI updates.

Pipes are simple, lightweight, supported directly in Linux, and work naturally with processes created using `fork()`.

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

Message meaning:

| Field          | Meaning                          |
| -------------- | -------------------------------- |
| `type`         | Message type                     |
| `pid`          | Child process ID                 |
| `traveler_id`  | Traveler index                   |
| `current_node` | Node reached by the traveler     |
| `next_node`    | Next node, or destination marker |

---

## Milestone 5 Log Example

```text
[PID=1021] arrived at node 0 | next node: 2
[PID=1022] arrived at node 2 | next node: 1
[PID=1021] arrived at node 4 | DESTINATION
[PID=1022] finished
[PID=1021] finished
```

All log messages are printed by the parent process only.

---

## Notes

* Written in C
* Designed for Linux
* GUI uses Raylib
* Shortest paths are calculated with Dijkstra
* Milestone 5 uses pipes for IPC
* Makefile includes targets for all milestones
