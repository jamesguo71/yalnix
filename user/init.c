#include <yuser.h>

int main(int argc, char **argv) {
    while(1) {
        int pid = GetPid();
        TracePrintf(1, "Init pid: %d. Pausing.\n", pid);
        Pause();
    }
    return 0;
}