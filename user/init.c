#include <yuser.h>

int main(int argc, char **argv) {
    int flag = 0;
    while(1) {
        if (flag) {
            TracePrintf(1, "Init delaying for 2 clock cycles\n");
            Delay(2);
            flag = 0;
        } else {
            TracePrintf(1, "Init pausing\n");
            Pause();
            flag = 1;
        }
    }
    return 0;
}