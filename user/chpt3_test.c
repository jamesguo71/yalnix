#include <yuser.h>

char *g_hello_string = "Hello, World!";

int main(int argc, char **argv) {
    // 1. Start off by testing our GetPid syscall.
    int pid = GetPid();
    TracePrintf(1, "Checkpoint 3 test program pid: %d\n", pid);

    // 2. Now let's trying delaying for several cycles
    int delay = 3;
    TracePrintf(1, "Checkpoint 3 test program delaying for %d cycles\n", delay);
    Delay(delay);

    // 3. Now let's test out BRK by mallocing and using memory.
    TracePrintf(1, "Checkpoint 3 testing BRK by mallocing memory...\n");
    char *hello = (char *) malloc(strlen(g_hello_string) + 1);
    strcpy(hello, g_hello_string);
    TracePrintf(1, "Copied string: %s\n", hello);

    // 4. Let's try some more memory...
    TracePrintf(1, "Checkpoint 3 mallocing a lot of memory...\n");
    int numbers_len = 5000;
    int *numbers = (int *) malloc(numbers_len * sizeof(int));
    for (int i = 0; i < numbers_len; i++) {
        numbers[i] = i;
    }

    int numbers_sum = 0;
    for (int i = 0; i < numbers_len; i++) {
        numbers_sum += numbers[i];
    }
    TracePrintf(1, "Numbers sum: %d\n", numbers_sum);

    // 5. Free our numbers array. NOTE: I don't think malloc actually gives memory back so
    //    we will most likely NOT see our syscall BRK called to reduce the brk.
    free(numbers);

    // 6. Loop forever.
    while(1) {
        TracePrintf(1, "Checkpoint 3 pid: %d delaying for %d cycles\n", pid, delay);
        Delay(delay);
    }
    return 0;
}