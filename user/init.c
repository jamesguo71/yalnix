#include <yuser.h>

int main(int argc, char **argv) {
    while(1) {
        int pid = GetPid();
        TracePrintf(1, "Init pid: %d. Delaying for 2 clock cycles\n", pid);
        Delay(2);
    }
    return 0;
}