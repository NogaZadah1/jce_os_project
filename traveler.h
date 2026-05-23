#ifndef TRAVELER_H
#define TRAVELER_H

#include <sys/types.h>

typedef struct {
    int source;
    int destination;

    int *path;
    int path_length;

    pid_t pid;
    int finished;
} Traveler;

#endif