#include <yuser.h>

int main(int argc, char **argv) {
    int answer = 42 / 0;
    TracePrintf(1, "[trap_math] The answer is: %d\n", answer);
    return 0;
}