# README
CMP SCI 4760 - Project 2
Curtis Been

This project simulates a system clock using shared memory. The oss program launches multiple worker processes that terminate based on a calculated simulated time. Each worker checks the clock frequently and prints output until its time expires.

## Compilation:
make

## Run example:
./oss -n 5 -s 2 -t 4 -i 100

## Cleanup:
If shared memory remains:
ipcs -m
ipcrm -m <shmid>

