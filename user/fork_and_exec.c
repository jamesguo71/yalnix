#include "yuser.h"

int main() {
    int pid = Fork();
    if (pid) {
        while (1) {
            TracePrintf(1, "[fork_and_exec] Parent forked child: %d\n", pid);
            Pause();
        }
    } else {
        char *program   = "./user/tty_test";
        char *argvec[1];
        argvec[0]       = "./user/tty_test";
        TracePrintf(1, "[fork_and_exec] Child about to exec: %s \n", program);
        Exec(program, argvec);
        while (1) {
            TracePrintf(1, "[fork_and_exec] Child failed to exec: %s\n");
            Pause();
        }
    }
    return 0;
}