#include "yuser.h"

int main() {
    int pipe_id;
    PipeInit(&pipe_id);
    TracePrintf(1, "[reclaim_test.c] Initialized Pipe with id = %d\n", pipe_id);
    int pid1 = Fork();
    if (pid1 == 0) {
        TracePrintf(1, "[reclaim_test.c] In Child\n");
        int size = 20;
        char read_buf[size];
        PipeRead(pipe_id, read_buf, size);
        TracePrintf(1, " [reclaim_test.c] Received: %s", read_buf);
        Exit(0);
    }

    char write_buf[20];
    sprintf(write_buf, "hello child!\n");
    PipeWrite(pipe_id, write_buf, strlen(write_buf));

    int pipe_id_2;
    PipeInit(&pipe_id_2);
    TracePrintf(1, "[reclaim_test.c] Initialized another Pipe id = %d\n", pipe_id_2);

    int pid2 = Fork();
    if (pid2 == 0) {
        TracePrintf(1, "[reclaim_test.c] In Child\n");
        int size = 20;
        char read_buf[size];
        PipeRead(pipe_id_2, read_buf, size);
        TracePrintf(1, "[reclaim_test.c] Child Received: %s", read_buf);
        Exit(0);
    }
    sprintf(write_buf, "Second hello!\n");
    PipeWrite(pipe_id_2, write_buf, strlen(write_buf));

    while (Wait(NULL) != ERROR);
    Reclaim(pipe_id);
    Reclaim(pipe_id_2);

//    int pipe_ids[1025];
//    for (int i = 0; i < 1025; i++) {
//        if (PipeInit(&pipe_ids[i]) != ERROR) {
//            TracePrintf(1, "[reclaim_test.c] Initialized Pipe id = %d\n", pipe_ids[i]);
//        }
//    }
//
//    for (int i = 0; i < 1024; i++) {
//        TracePrintf(1, "[reclaim_test.c] Reclaim Pipe id = %d\n", pipe_ids[i]);
//        Reclaim(pipe_ids[i]);
//    }

    int lock_id;
    LockInit(&lock_id);
    TracePrintf(1, "[reclaim_test.c] Initialized lock_id: %d\n", lock_id);
    int lock_id2;
    LockInit(&lock_id2);
    TracePrintf(1, "[reclaim_test.c] Initialized lock_id: %d\n", lock_id2);
    Reclaim(lock_id);
    Reclaim(lock_id2);
    int lock_id3;
    LockInit(&lock_id3);
    TracePrintf(1, "[reclaim_test.c] Initialized lock_id: %d\n", lock_id3);
    Reclaim(lock_id3);
}