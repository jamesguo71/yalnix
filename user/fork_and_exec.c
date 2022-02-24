#include "yuser.h"

int main() {
    int pid = Fork();
    if (pid) {
        while (1) {
            TracePrintf(1, "[fork_and_exec] Parent forked child: %d\n", pid);
            Pause();
        }
    } else {
        char *program   = "./user/chpt3_test.c";
        char *argvec[1];
        argvec[0]       = "./user/chpt3_test.c";
        TracePrintf(1, "[fork_and_exec] Child about to exec: %s \n", program);
        Exec(program, argvec);
        while (1) {
            TracePrintf(1, "[fork_and_exec] Child failed to exec: %s\n");
            Halt();
        }
    }
    return 0;
}