#include "yuser.h"

#define READ_LEN  100
#define WRITE_LEN 1000

int main() {
    // 1. Fork and have the parent/child take turns reading and writing to one another.
    int pipe = PipeInit(&pipe);
    int pid  = Fork();
    if (pid) {
        char read_buf[READ_LEN];
        while (1) {
            TracePrintf(1, "[pipe_test] Parent reading from pipe: %d \n", pipe);
            int read_len = PipeRead(pipe, (void *) read_buf, READ_LEN);
            read_buf[read_len - 1] = '\0';
            TracePrintf(1, "[pipe_test] Parent read: %s\n", read_buf);
            Pause();
        }
    } else {
        char write_buf[WRITE_LEN];
        for (int i = 0; i < WRITE_LEN; i++) {
            write_buf[i] = (i % 26) + 97;
        }
        write_buf[WRITE_LEN - 1] = '\0';
        while (1) {
            TracePrintf(1, "[pipe_test] Child writing to pipe: %d \n", pipe);
            PipeWrite(pipe, (void *) write_buf, WRITE_LEN);
            TracePrintf(1, "[pipe_test] Child wrote: %s\n", write_buf);
            Pause();
        }
    }
    return 0;
}