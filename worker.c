// worker.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "shared.h"

#define SHM_KEY 1234

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./worker seconds nanoseconds\n");
        exit(1);
    }

    int maxSec = atoi(argv[1]);
    int maxNano = atoi(argv[2]);

    int shm_id = shmget(SHM_KEY, sizeof(SimClock), 0666);
    if (shm_id == -1) {
        perror("worker shmget");
        exit(1);
    }

    SimClock *clock = (SimClock*) shmat(shm_id, NULL, 0);
    unsigned int termSec = clock->seconds + maxSec;
    unsigned int termNano = clock->nanoseconds + maxNano;
    if (termNano >= BILLION) {
        termSec++;
        termNano -= BILLION;
    }

    unsigned int lastPrintSec = clock->seconds;
    printf("WORKER PID:%d PPID:%d SysClockS:%u SysclockNano:%u TermTimeS:%u TermTimeNano:%u\n--Just Starting\n", getpid(), getppid(), clock->seconds, clock->nanoseconds, termSec, termNano);

    while (1) {
        if (clock->seconds > termSec || (clock->seconds == termSec && clock->nanoseconds >= termNano)) {
            printf("WORKER PID:%d PPID:%d SysClockS:%u SysclockNano:%u TermTimeS:%u TermTimeNano:%u\n--Terminating\n", getpid(), getppid(), clock->seconds, clock->nanoseconds, termSec, termNano);
            break;
        }
        if (clock->seconds > lastPrintSec) {
            lastPrintSec = clock->seconds;
            printf("WORKER PID:%d PPID:%d SysClockS:%u SysclockNano:%u TermTimeS:%u TermTimeNano:%u\n--%u seconds have passed since starting\n", getpid(), getppid(), clock->seconds, clock->nanoseconds, termSec, termNano, clock->seconds - (termSec - maxSec));
        }
    }

    shmdt(clock);
    return 0;
}
