CC = gcc
CFLAGS = -Wall -Wextra -std=c99
RAYLIB_FLAGS = -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

M4_SRC = main_m4.c graph.c file_reader.c dijkstra.c gui.c traveler.c

milestone4:
	$(CC) $(CFLAGS) $(M4_SRC) -o sim $(RAYLIB_FLAGS)

clean:
	rm -f sim *.o