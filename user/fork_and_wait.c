#include "yuser.h"

int main() {
    int pid = Fork();
    if (pid) {
        TracePrintf(1, "[fork_and_wait] Parent waiting on child: %d\n", pid);
        int status;
        Wait(&status);
        while (1) {
            TracePrintf(1, "[fork_and_wait] Parent received child's exit status: %d\n", status);
            Pause();
        }
    }
    TracePrintf(1, "[fork_and_wait] Child fork return value: %d \n", pid);
    return 0;
}