#include "yuser.h"

#define BUF_LEN 10

int main() {
    // 1. Declare a buffer for reading/writing and initialize a pipe
    int  pipe = PipeInit(&pipe);
    char buf[BUF_LEN];
    buf[0] = 'a';
    buf[1] = '\0';

    // 2. Fork and have the parent/child take turns reading and writing to one another.
    int pid  = Fork();
    if (pid) {
        while (1) {
            TracePrintf(1, "[pipe_test] Parent reading from pipe: %d \n", pipe);
            PipeRead(pipe, (void *) buf, BUF_LEN);

            TracePrintf(1, "[pipe_test] Parent read: %s\n", buf);
            buf[0] += 1;

            TracePrintf(1, "[pipe_test] Parent writing to pipe: %d \n", pipe);
            PipeWrite(pipe, (void *) buf, BUF_LEN);

            TracePrintf(1, "[pipe_test] Parent wrote: %s\n", buf);
            Pause();
        }
    } else {
        while (1) {
            TracePrintf(1, "[pipe_test] Child writing to pipe: %d \n", pipe);
            PipeWrite(pipe, (void *) buf, BUF_LEN);

            TracePrintf(1, "[pipe_test] Child wrote: %s\n", buf);
            Pause();

            TracePrintf(1, "[pipe_test] Child reading from pipe: %d \n", pipe);
            PipeRead(pipe, (void *) buf, BUF_LEN);

            TracePrintf(1, "[pipe_test] Child read: %s\n", buf);
            buf[0] += 1;
        }
    }
    return 0;
}