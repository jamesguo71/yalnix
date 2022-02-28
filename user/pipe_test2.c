#include "yuser.h"

#define READ_LEN 100

char *write_string = "Write the len bytes starting at buf to the named pipe.\n(As the pipe is a"
                     "FIFO buffer, these bytes should be appended to the sequence of unread bytes"
                     "currently in the pipe.)\nReturn as soon as you get the bytes into the buffer"
                     ".\nIn case of any error, the value ERROR is returned.\nOtherwise, return the"
                     "number of bytes written.";

int main() {
    // 1. Declare a buffer for reading/writing and initialize a pipe
    int  pipe = PipeInit(&pipe);

    // 2. Fork and have the parent/child take turns reading and writing to one another.
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
        while (1) {
            TracePrintf(1, "[pipe_test] Child writing to pipe: %d \n", pipe);
            PipeWrite(pipe, (void *) write_string, strlen(write_string));
            TracePrintf(1, "[pipe_test] Child wrote: %s\n", write_string);
            Pause();
        }
    }
    return 0;
}