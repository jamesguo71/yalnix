#include "yuser.h"

int main() {
    int pid = Fork();
    if (pid) {
        while (1) {
            TracePrintf(1, "[fork_and_exit] parent got pid returned from fork %d\n", pid);
            Pause();
        }
    }
    TracePrintf(1, "[fork_and_exit] child got pid returned from fork %d \n", pid);
    return 0;
}