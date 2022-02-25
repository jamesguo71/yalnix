#include "yuser.h"


/*
 * This program tests both (1) our ability to spawn grandchildren processes (2) and our correct
 * handling of parent processes that exit before their children. Specifically, the child process
 * in this program exits first, but is only "terminated" as its parent is still alive and waiting
 * on it. Thus, we delay the grandchild process so that the parent has time to unblock from wait
 * and retrieve the child's information (which also involves completely deleting the child process'
 * pcb information and removing its pointer in the grandchild pcb). Then, when the grandchild
 * finally runs again and exits, its parent (the child process) has already exited. So the
 * grandchild completely deletes itself instead of adding its pcb to the terminated queue.
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
            Delay(2);
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