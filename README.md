# OS Project - Milestone 4

## Overview

This project implements a directed weighted graph traffic simulation in C using Raylib.

The system reads a graph from an input file, computes shortest paths using Dijkstra's algorithm, and visualizes travelers moving along their routes.

Milestone 4 extends the project to support multiple travelers running simultaneously using Linux child processes created with `fork()`.

---

## Files

| File                             | Description                                          |
| -------------------------------- | ---------------------------------------------------- |
| `graph.c`, `graph.h`             | Graph representation using adjacency lists           |
| `file_reader.c`, `file_reader.h` | Reading graph and traveler data from input files     |
| `dijkstra.c`, `dijkstra.h`       | Dijkstra shortest path algorithm                     |
| `gui.c`, `gui.h`                 | Graph visualization and animation using Raylib       |
| `traveler.c`, `traveler.h`       | Traveler data structures and child process utilities |
| `main_m4.c`                      | Main program for Milestone 4                         |
| `input_m4.txt`                   | Example input file                                   |
| `Makefile`                       | Build targets                                        |

---

## Build and Run

Compile:

```bash
make clean
make milestone4
```

Run:

```bash
./sim input_m4.txt
```

---

## Input Format

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

## Milestone 4 Features

### Multiple Travelers

The program supports multiple travelers moving simultaneously on the graph.

Each traveler has:

* Source node
* Destination node
* Shortest path calculated using Dijkstra
* Independent animation state

### Parent Process

The parent process:

* Reads the graph and traveler definitions
* Computes shortest paths for all travelers
* Creates child processes using `fork()`
* Controls the Raylib GUI
* Updates traveler positions
* Terminates child processes when travelers finish

### Child Processes

One child process is created for each traveler.

The child process remains alive while the traveler is active and is terminated by the parent process when the route is completed.

### Animation

Travelers move along the shortest path returned by Dijkstra.

Travel time is proportional to edge weight.

Multiple travelers may appear on the graph at the same time.

---

## Notes

* Written in C
* Designed for Linux
* GUI implemented using Raylib
* Graph represented using adjacency lists
* Shortest paths calculated using Dijkstra's algorithm
* Multiple travelers implemented using `fork()`
