CC = gcc
CFLAGS = -Wall -Wextra -std=c99

M1_SRC = main_dijkstra.c graph.c file_reader.c dijkstra.c

milestone1:
	$(CC) $(CFLAGS) $(M1_SRC) -o dijkstra

clean:
	rm -f dijkstra *.o