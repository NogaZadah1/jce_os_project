#ifndef PARENT_CONTROLLER_H
#define PARENT_CONTROLLER_H

int run_milestone5(const char* input_file);

/*
 * Runs milestone 6 with node synchronization.
 * The parent creates shared node synchronization, starts autonomous travelers,
 * receives IPC state updates, and updates the GUI.
 */
int run_milestone6(const char* input_file);

#endif
