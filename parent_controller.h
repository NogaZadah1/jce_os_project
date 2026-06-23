#ifndef PARENT_CONTROLLER_H
#define PARENT_CONTROLLER_H

int run_milestone5(const char* input_file);

/*
 * Runs milestone 6 with node synchronization.
 * The parent creates shared node synchronization, starts autonomous travelers,
 * receives IPC state updates, and updates the GUI.
 */
int run_milestone6(const char* input_file);

typedef enum {
    SCHEDULING_FCFS,
    SCHEDULING_SJF
} SchedulingAlgorithm;

/*
 * Runs milestone 7 with parent-controlled node scheduling.
 * The selected algorithm determines which waiting traveler
 * receives permission to enter each node.
 */
int run_milestone7(
    const char* input_file,
    SchedulingAlgorithm algorithm
);

#endif