#include "yuser.h"

int main() {
    int ret;
    ret = Fork();
    int sem_id;
    SemInit(&sem_id, 2);
    if (ret < 0) return ERROR;
    if (ret == 0) {
        while (1) {
            SemDown(sem_id);
            TracePrintf(1, "first child: I got the semaphore and I will sleep!\n");
            Delay(10);
            TracePrintf(1, "first child: I woke up! I will up the semaphore and I'll sleep again.!\n");
            SemUp(sem_id);
            Delay(10);
        }
        Exit(0);
    }
    ret = Fork();
    if (ret == 0) {
        while (1) {
            SemDown(sem_id);
            TracePrintf(1, "second child: I got the semaphore and I will sleep!\n");
            Delay(10);
            TracePrintf(1, " second child:I woke up!  I will up the semaphore and I'll sleep again.!\n");
            SemUp(sem_id);
            Delay(10);
        }
        Exit(0);
    }

    while (1) {
        SemDown(sem_id);
        TracePrintf(1, "parent: I got the semaphore and I will sleep!\n");
        Delay(10);
        TracePrintf(1, "parent: I woke up! I will up the semaphore and I'll sleep again.!\n");
        SemUp(sem_id);
        Delay(10);
    }
}