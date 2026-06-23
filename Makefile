CC = gcc

RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

COMMON_SRC = graph.c dijkstra.c
GUI_SRC = gui.c
M4_SRC = file_reader.c traveler.c ipc.c node_sync.c
M5_SRC = file_reader.c traveler.c ipc.c parent_controller.c m5_gui_adapter.c node_sync.c scheduler.c
M6_SRC = file_reader.c traveler.c ipc.c parent_controller.c m5_gui_adapter.c node_sync.c scheduler.c
M7_SRC = file_reader.c traveler.c ipc.c parent_controller.c m5_gui_adapter.c node_sync.c scheduler.c
milestone1:
	$(CC) main_dijkstra.c $(COMMON_SRC) file_reader.c -o dijkstra

milestone2:
	$(CC) main_static.c $(COMMON_SRC) $(GUI_SRC) file_reader.c -o sim $(RAYLIB_FLAGS)

milestone3:
	$(CC) main_sim.c $(COMMON_SRC) $(GUI_SRC) file_reader.c -o sim $(RAYLIB_FLAGS)

milestone4:
	$(CC) main_m4.c $(COMMON_SRC) $(GUI_SRC) $(M4_SRC) -o sim $(RAYLIB_FLAGS)

milestone5:
	$(CC) main_m5.c $(COMMON_SRC) $(GUI_SRC) $(M5_SRC) -o sim $(RAYLIB_FLAGS)

milestone6:
	$(CC) main_m6.c $(COMMON_SRC) $(GUI_SRC) $(M6_SRC) -o sim $(RAYLIB_FLAGS)

milestone7:
	$(CC) main_m7.c $(COMMON_SRC) $(GUI_SRC) $(M7_SRC) -o sim $(RAYLIB_FLAGS)

test_reader:
	$(CC) test_reader.c graph.c file_reader.c -o test_reader

clean:
	rm -f dijkstra static_gui sim m4_sim test_reader *.o