CC = gcc
CFLAGS = -Wall -Wextra -std=c99

RAYLIB_FLAGS = -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

M3_SRC = main_sim.c graph.c file_reader.c dijkstra.c gui.c

milestone3:
	$(CC) $(CFLAGS) $(M3_SRC) -o sim $(RAYLIB_FLAGS)

clean:
	rm -f sim *.o