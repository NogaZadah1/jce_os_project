//
// Created by student on 08/06/2026.
//
#include <stdio.h>

#include "parent_controller.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./sim <file_name>\n");
        return 1;
    }

    return run_milestone6(argv[1]);
}