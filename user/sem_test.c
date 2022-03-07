#include "yuser.h"

int main() {
    int sem_id;
    if (SemInit(&sem_id, 1) == ERROR)
        TracePrintf(1, "[sem_test.c] error in SemInit\n");
    TracePrintf(1, "[sem_test.c] Init sem_id = %d\n", sem_id);
    int ret;
    ret = Fork();
    if (ret < 0)
        TracePrintf(1, "[sem_test.c] error in Fork\n");
    if (ret == 0) {
        SemDown(sem_id);
        TracePrintf(1, "[sem_test.c] Child got lock!\n");
        for (int i = 0; i < 10; i++) {
            TracePrintf(1, "[sem_test.c] Child will delay\n");
            Delay(1);
        }
        SemUp(sem_id);
        Exit(0);
    }
    SemDown(sem_id);
    TracePrintf(1, "[sem_test.c] Parent got lock.\n");
    for (int i = 0; i < 10; i++) {
        TracePrintf(1, "[sem_test.c] Parent will delay.\n");
        Delay(1);
    }
    SemUp(sem_id);

    Wait(NULL);
    Reclaim(sem_id);

    int sem_id2;
    if (SemInit(&sem_id2, 1) == ERROR)
        TracePrintf(1, "[sem_test.c] error in SemInit\n");
    TracePrintf(1, "[sem_test.c] Init second sem_id = %d\n", sem_id2);
    Reclaim(sem_id2);
}