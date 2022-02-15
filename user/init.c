#include <yuser.h>

int main(int argc, char **argv) {
    while(1) {
        TracePrintf(1, "Init delaying for 2 clock cycles\n");
        Delay(2);
    }
    return 0;
}