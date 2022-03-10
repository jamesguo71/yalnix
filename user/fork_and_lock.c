#include "yuser.h"

int main() {
    int lock_id;
    LockInit(&lock_id);

    int ret;
    ret = Fork();
    TracePrintf(1, "After Fork...\n");
    if (ret < 0) {
        TracePrintf(1, "Fork error\n");
        return ERROR;
    }
    if (ret == 0) {
        while (1) {
            Acquire(lock_id);
            TracePrintf(1, "first child: I got the lock and I will sleep!\n");
            Delay(10);
            TracePrintf(1, "first child: I woke up! I will release the lock and I'll sleep again.!\n");
            Release(lock_id);
            Delay(10);
        }
        Exit(0);
    }
    ret = Fork();
    if (ret == 0) {
        while (1) {
            Acquire(lock_id);
            TracePrintf(1, "second child: I got the lock and I will sleep!\n");
            Delay(10);
            TracePrintf(1, " second child:I woke up!  I will release the lock and I'll sleep again.!\n");
            Release(lock_id);
            Delay(10);
        }
        Exit(0);
    }

    while (1) {
        Acquire(lock_id);
        TracePrintf(1, "parent: I got the lock and I will sleep!\n");
        Delay(10);
        TracePrintf(1, "parent: I woke up! I will release the lock and I'll sleep again.!\n");
        Release(lock_id);
        Delay(10);
    }
}