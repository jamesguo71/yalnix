#include <yuser.h>

void recurse(int depth) {
    TracePrintf(1, "[trap_memory] Recurse depth: %d\n", depth);
    int array[1000];
    recurse(--depth);
}

int main() {
    int depth = 10;
    recurse(depth);
    return 0;
}