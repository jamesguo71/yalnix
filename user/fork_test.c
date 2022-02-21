#include "yuser.h"

int main() {
    int pid;
    pid = Fork();
    if (pid == 0) {
        while (1) {
            TracePrintf(1, "[fork_test] child pid %d \n", pid);
            Pause();
        }
    }
    while (1) {
        TracePrintf(1, "[fork_test] parent pid %d\n", pid);
        Pause();
    }
    return 0;
}