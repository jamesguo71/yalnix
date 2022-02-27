#include "yuser.h"

int main() {
    int pid = Fork();
    if (pid) {
        while (1) {
            TracePrintf(1, "[tty_test_wrapper] Parent forked child: %d\n", pid);
            Delay(100000);
        }
    } else {
        char *program   = "./user/tty_test";
        char *argvec[1];
        argvec[0]       = "./user/tty_test";
        TracePrintf(1, "[tty_test_wrapper] Child about to exec: %s \n", program);
        Exec(program, argvec);
        while (1) {
            TracePrintf(1, "[tty_test_wrapper] Child failed to exec: %s\n");
            Pause();
        }
    }
    return 0;
}