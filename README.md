# OS Project - Graph Traffic Simulation

This project implements a directed weighted graph, Dijkstra shortest path algorithm, static graph visualization, and animated movement on the graph using raylib.

## Files

- `graph.c`, `graph.h` - graph representation using adjacency lists
- `file_reader.c`, `file_reader.h` - reading graph input from text file
- `dijkstra.c`, `dijkstra.h` - Dijkstra shortest path algorithm
- `main_dijkstra.c` - milestone 1 main file
- `main_static.c` - milestone 2 static GUI main file
- `main_sim.c` - milestone 3 animation main file
- `input.txt` - example input file
- `Makefile` - build targets for milestones

## Input Format

```txt
N M
src dst weight
...
src dst