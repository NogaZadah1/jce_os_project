cmake_minimum_required(VERSION 4.2)
project(graph_module C)

set(CMAKE_C_STANDARD 11)

add_executable(graph_module main.c
        graph.c
        graph.h
        file_reader.c
        file_reader.h
)
