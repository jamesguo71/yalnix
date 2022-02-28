#include "yuser.h"

#define BUF_LEN  1024
#define READ_LEN 100

int main() {
    // 1. Declare a buffer for reading/writing and initialize a pipe
    int  pipe = PipeInit(&pipe);
    char buf[BUF_LEN];
    for (int i = 0; i < BUF_LEN; i++) {
        buf[i] = 'a';
    }
    buf[BUF_LEN - 1] = '\0';

    // 2. Fork and have the parent/child take turns reading and writing to one another.
    int pid  = Fork();
    if (pid) {
        while (1) {
            TracePrintf(1, "[pipe_test] Parent reading from pipe: %d \n", pipe);
            PipeRead(pipe, (void *) buf, READ_LEN);
            buf[READ_LEN - 1] = '\0';
            TracePrintf(1, "[pipe_test] Parent read: %s\n", buf);
            Pause();
        }
    } else {
        while (1) {
            TracePrintf(1, "[pipe_test] Child writing to pipe: %d \n", pipe);
            PipeWrite(pipe, (void *) buf, BUF_LEN);
            TracePrintf(1, "[pipe_test] Child wrote: %s\n", buf);
            Pause();
        }
    }
    return 0;
}