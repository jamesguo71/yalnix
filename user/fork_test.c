#include "yuser.h"

int main() {
    int pid;
    pid = Fork();
    if (pid == 0) {
        while (1) {
            TracePrintf(1, "[fork_test] child got pid returned from fork %d \n", pid);
            Pause();
        }
    }
    while (1) {
        TracePrintf(1, "[fork_test] parent got pid returned from fork %d\n", pid);
        Pause();
    }
    return 0;
}