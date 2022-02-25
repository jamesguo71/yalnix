#include <yuser.h>

int main() {
    int pid = Fork();
    if (pid) {
        while (1) {
            TracePrintf(1, "[trap_math] Parent forked child: %d\n", pid);
            Delay(2);
        }
    }
    int answer = 42 / 0;
    TracePrintf(1, "[trap_math] Child divided by zero answer is: %d\n", answer);
    return 0;
}