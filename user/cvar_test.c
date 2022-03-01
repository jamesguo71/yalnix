#include "yuser.h"

#define NUM_CHILDREN 10

int main() {
    // 1. Create a new cvar and lock. Exit upon error.
    int cvar;
    int ret = CvarInit(&cvar);
    if (ret < 0) {
        TracePrintf(1, "[cvar_test] Error initializing cvar. Exiting...\n");
        return ret;
    }
    int lock;
    ret = LockInit(&lock);
    if (ret < 0) {
        TracePrintf(1, "[cvar_test] Error initializing lock. Exiting...\n");
        return ret;
    }

    // 2. Spawn a number of child processes, where the child processes delay and then attempt to
    //    aquire the lock. Once they have it, they print a message, release the lock, and exit.
    for (int i = 0; i < NUM_CHILDREN; i++) {
        int pid = Fork();
        if (pid) {
            TracePrintf(1, "[cvar_test] Parent forked child process: %d\n", pid);
        } else {

            // Child process gets its pid and tries to aquire the lock
            pid = GetPid();
            Acquire(lock);

            // If the child process pid is odd, then it should give up the lock and
            // wait for a signal on the cvar before proceeding.
            if (pid % 2) {
                TracePrintf(1, "[cvar_test] Child %d waiting on cvar %d\n", pid, cvar);
                CvarWait(cvar, lock);
            }
            TracePrintf(1, "[cvar_test] Child %d signaling cvar %d\n", pid, cvar);
            CvarSignal(cvar);
            Delay(pid);
            Release(lock);
            return pid;
        }
    }

    // 3. After spawning the child processes, the parent should wait for them all to finish.
    for (int i = 0; i < NUM_CHILDREN; i++) {
        TracePrintf(1, "[cvar_test] Parent waiting on any child to finish...\n");
        int status;
        int pid = Wait(&status);
        TracePrintf(1, "[cvar_test] Parent received child: %d exit status: %d\n", pid, status);
    }
    return 0;
}