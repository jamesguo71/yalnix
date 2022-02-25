#include <yuser.h>

int main() {
    // parent process
    int pid = Fork();
    if (pid) {
        TracePrintf(1, "[fork_and_wait] Parent waiting on child: %d to finish...\n", pid);
        int status;
        pid = Wait(&status);
        while(1) {
            TracePrintf(1, "[fork_and_wait] Parent received child: %d exit status: %d\n",
                                            pid, status);
            Delay(2);
        }
    }

    // child process
    int answer = 42 / 0;
    TracePrintf(1, "[trap_math] Child divided by zero answer is: %d\n", answer);
    return 0;
}