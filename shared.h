// shared.h
#ifndef SHARED_H
#define SHARED_H

#include <sys/types.h>

#define MAX_PROCESSES 20
#define BILLION 1000000000

// Simulated clock structure
typedef struct {
    unsigned int seconds;
    unsigned int nanoseconds;
} SimClock;

#endif
