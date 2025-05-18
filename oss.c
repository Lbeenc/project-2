// oss.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "shared.h"

#define SHM_KEY 1234

struct PCB {
    int occupied;
    pid_t pid;
    int startSeconds;
    int startNano;
};

struct PCB processTable[MAX_PROCESSES];
SimClock *simClock;
int shm_id;
int maxProcesses = 5, simul = 2, timeLimit = 5, launchInterval = 100; // Defaults
int activeChildren = 0, totalLaunched = 0;
int running = 1;
time_t startTime;

void incrementClock(int ns) {
    simClock->nanoseconds += ns;
    if (simClock->nanoseconds >= BILLION) {
        simClock->seconds++;
        simClock->nanoseconds -= BILLION;
    }
}

void handleSignal(int sig) {
    printf("\n[OSS] Caught signal %d. Cleaning up...\n", sig);
    running = 0;
}

void cleanup() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processTable[i].occupied)
            kill(processTable[i].pid, SIGTERM);
    }
    shmdt(simClock);
    shmctl(shm_id, IPC_RMID, NULL);
}

int findFreePCBSlot() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processTable[i].occupied)
            return i;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: oss [-n proc] [-s simul] [-t timelimit] [-i interval(ms)]\n");
                exit(0);
            case 'n': maxProcesses = atoi(optarg); break;
            case 's': simul = atoi(optarg); break;
            case 't': timeLimit = atoi(optarg); break;
            case 'i': launchInterval = atoi(optarg); break;
            default:
                fprintf(stderr, "Invalid option.\n");
                exit(1);
        }
    }

    signal(SIGINT, handleSignal);
    alarm(60);
    signal(SIGALRM, handleSignal);

    shm_id = shmget(SHM_KEY, sizeof(SimClock), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    simClock = (SimClock*) shmat(shm_id, NULL, 0);
    simClock->seconds = 0;
    simClock->nanoseconds = 0;
    startTime = time(NULL);

    memset(processTable, 0, sizeof(processTable));
    unsigned long nextLaunchTime = 0;
    unsigned long nextPrintTime = 0;

    while (running && (totalLaunched < maxProcesses || activeChildren > 0)) {
        usleep(5000); // simulate passage of real time
        incrementClock(50000);

        int pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0) {
            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (processTable[i].occupied && processTable[i].pid == pid) {
                    processTable[i].occupied = 0;
                    activeChildren--;
                    break;
                }
            }
        }

        unsigned long currentTime = (unsigned long)simClock->seconds * BILLION + simClock->nanoseconds;

        if (currentTime >= nextLaunchTime && activeChildren < simul && totalLaunched < maxProcesses) {
            int slot = findFreePCBSlot();
            if (slot != -1) {
                int sec = rand() % timeLimit + 1;
                int nano = rand() % 1000000000;

                char secStr[10], nanoStr[15];
                sprintf(secStr, "%d", sec);
                sprintf(nanoStr, "%d", nano);

                pid_t cpid = fork();
                if (cpid == 0) {
                    execl("./worker", "./worker", secStr, nanoStr, NULL);
                    perror("execl");
                    exit(1);
                } else {
                    printf("[OSS] Launched worker PID %d\n", cpid);
                    processTable[slot].occupied = 1;
                    processTable[slot].pid = cpid;
                    processTable[slot].startSeconds = simClock->seconds;
                    processTable[slot].startNano = simClock->nanoseconds;
                    totalLaunched++;
                    activeChildren++;
                    nextLaunchTime = currentTime + (launchInterval * 1000000);
                }
            }
        }

        if (currentTime >= nextPrintTime) {
            printf("\nOSS PID:%d SysClockS:%u SysClockNano:%u\nProcess Table:\n", getpid(), simClock->seconds, simClock->nanoseconds);
            printf("Entry Occupied PID StartS StartN\n");
            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (processTable[i].occupied)
                    printf("%d %d %d %d %d\n", i, processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds, processTable[i].startNano);
            }
            nextPrintTime = currentTime + 500000000;
        }
    }

    cleanup();
    return 0;
}
