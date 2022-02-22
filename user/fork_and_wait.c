#include "yuser.h"


/*
 * The parent process spawns num_children child processes then waits for them to exit.
 * The child processes simply print their pid and exit. Afterwards, the parent exits as
 * well, which should cause a system halt since the parent is the init program.
 */
int main() {
    int num_children = 5;
    for (int i = 0; i < num_children; i++) {
        int pid = Fork();
        if (pid) {
            TracePrintf(1, "[fork_and_wait] Parent forked child process: %d\n", pid);
        } else {
            pid = GetPid();
            TracePrintf(1, "[fork_and_wait] Child %d exiting...\n", pid);
            return 0;
        }
    }

    for (int i = 0; i < num_children; i++) {
        TracePrintf(1, "[fork_and_wait] Parent waiting on any child to finish...\n");
        int status;
        int pid = Wait(&status);
        TracePrintf(1, "[fork_and_wait] Parent received child: %d exit status: %d\n", pid, status);
    }
    return 0;
}