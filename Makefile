CC = gcc

RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

COMMON_SRC = graph.c dijkstra.c
GUI_SRC = gui.c
M4_SRC = file_reader.c traveler.c

milestone1:
	$(CC) main_dijkstra.c $(COMMON_SRC) -o dijkstra

milestone2:
	$(CC) main_static.c $(COMMON_SRC) $(GUI_SRC) -o static_gui $(RAYLIB_FLAGS)

milestone3:
	$(CC) main_sim.c $(COMMON_SRC) $(GUI_SRC) -o sim $(RAYLIB_FLAGS)

milestone4:
	$(CC) main_m4.c $(COMMON_SRC) $(GUI_SRC) $(M4_SRC) -o sim $(RAYLIB_FLAGS)

test_reader:
	$(CC) test_reader.c graph.c file_reader.c -o test_reader

clean:
	rm -f dijkstra static_gui sim m4_sim test_reader