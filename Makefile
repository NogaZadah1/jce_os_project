CC = gcc
CFLAGS = -Wall -Wextra -std=c99
RAYLIB_FLAGS = -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

M2_SRC = main_static.c graph.c file_reader.c dijkstra.c gui.c

milestone2:
	$(CC) $(CFLAGS) $(M2_SRC) -o sim $(RAYLIB_FLAGS)

clean:
	rm -f sim *.o