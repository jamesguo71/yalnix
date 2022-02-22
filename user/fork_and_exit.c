#include "yuser.h"

int main() {
    int pid = Fork();
    if (pid) {
        while (1) {
            TracePrintf(1, "[fork_and_exit] Parent fork return value: %d\n", pid);
            Pause();
        }
    }
    TracePrintf(1, "[fork_and_exit] Child fork return value: %d \n", pid);
    return 0;
}