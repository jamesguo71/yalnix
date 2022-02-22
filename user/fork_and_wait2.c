#include "yuser.h"


/*
 * The parent process spawns num_children child processes then waits for them to exit.
 * The child processes simply print their pid and exit. Afterwards, the parent exits as
 * well, which should cause a system halt since the parent is the init program.
 */
int main() {
    // parent process
    int pid = Fork();
    if (pid) {
        TracePrintf(1, "[fork_and_wait] Parent waiting on child: %d to finish...\n", pid);
        int status;
        pid = Wait(&status);
        while(1) {
            TracePrintf(1, "[fork_and_wait] Parent received child: %d exit status: %d\n", pid, status);
            Pause();
        }
    } 

    // child process
    else {
        pid = Fork();
        if (pid) {
            TracePrintf(1, "[fork_and_wait] Child spawned grandchild: %d. Exiting...\n", pid);
            return 0;
        } 

        // grandchild process
        else {
            pid = GetPid();
            TracePrintf(1, "[fork_and_wait] Grandchild process: %d delaying...\n", pid);
            Delay(2);
            TracePrintf(1, "[fork_and_wait] Grandchild process: %d exiting...\n", pid);
            return 0;
        }
    }
    return 0;
}