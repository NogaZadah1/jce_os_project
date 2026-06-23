//
// Created by student on 22/06/2026.
//
#include <stdio.h>
#include <string.h>

#include "parent_controller.h"

int main(int argc, char* argv[]) {
    SchedulingAlgorithm algorithm;

    if (argc != 4 || strcmp(argv[1], "-schd") != 0) {
        fprintf(
            stderr,
            "Usage: ./sim -schd fcfs <file_name>\n"
            "   or: ./sim -schd sjf <file_name>\n"
        );
        return 1;
    }

    if (strcmp(argv[2], "fcfs") == 0) {
        algorithm = SCHEDULING_FCFS;
    } else if (strcmp(argv[2], "sjf") == 0) {
        algorithm = SCHEDULING_SJF;
    } else {
        fprintf(
            stderr,
            "Error: scheduling algorithm must be fcfs or sjf\n"
        );
        return 1;
    }

    return run_milestone7(argv[3], algorithm);
}