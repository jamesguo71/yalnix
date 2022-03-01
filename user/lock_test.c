#include "yuser.h"

#define NUM_CHILDREN 10

int main() {
    // 1. Create a new lock. Exit upon error.
    int lock = LockInit(&lock);
    if (lock < 0) {
        TracePrintf(1, "[lock_test] Error initializing lock. Exiting...\n");
        return lock;
    }

    // 2. Spawn a number of child processes, where the child processes delay and then attempt to
    //    aquire the lock. Once they have it, they print a message, release the lock, and exit.
    for (int i = 0; i < NUM_CHILDREN; i++) {
        int pid = Fork();
        if (pid) {
            TracePrintf(1, "[lock_test] Parent forked child process: %d\n", pid);
        } else {
            pid = GetPid();
            Acquire(lock);
            TracePrintf(1, "[lock_test] Child %d aquired lock %d\n", pid, lock);
            Delay(pid);
            Release(lock);
            return pid;
        }
    }

    // 3. After spawning the child processes, the parent should wait for them all to finish.
    for (int i = 0; i < NUM_CHILDREN; i++) {
        TracePrintf(1, "[lock_test] Parent waiting on any child to finish...\n");
        int status;
        int pid = Wait(&status);
        TracePrintf(1, "[lock_test] Parent received child: %d exit status: %d\n", pid, status);
    }
    return 0;
}