#include "yuser.h"

#define NUM_CHILDREN 10

int main() {
    // 1. Create a new cvar and lock. Exit upon error.
    int cvar;
    int ret = CvarInit(&cvar);
    if (ret < 0) {
        TracePrintf(1, "[cvar_test_2.c] Error initializing cvar. Exiting...\n");
        return ret;
    }
    int lock;
    ret = LockInit(&lock);
    if (ret < 0) {
        TracePrintf(1, "[cvar_test_2.c] Error initializing lock. Exiting...\n");
        return ret;
    }

    ret = Fork();
    if (ret < 0) return ERROR;
    if (ret == 0) {
        while (1) {
            Acquire(lock);
            TracePrintf(1, "[cvar_test_2.c] Child got the lock!\n");
            for (int i = 0; i < 100000; i++) {
                if (i % 10000 == 0){
                    TracePrintf(1, "[cvar_test_2.c] Child gets to %d\n", i);
                    Delay(1);
                }
                if (i == 89757) {
                    TracePrintf(1, "[cvar_test_2.c] Child Will send signal to cvar waiting processes\n");
                    CvarSignal(cvar);
                    TracePrintf(1, "[cvar_test_2.c] Child releases the lock\n");
                    Release(lock);
                    Delay(2);
                    break;
                }
            }
        }
    } else {
        while (1) {
            Acquire(lock);
            TracePrintf(1, "[cvar_test_2.c] Parent got the lock!\n");
            TracePrintf(1, "[cvar_test_2.c] Parent will wait for cvar and see if child does signal!\n");
            CvarWait(cvar, lock);
            TracePrintf(1, "[cvar_test_2.c] Parent got signal! Now will delay some time and release the lock\n");
            Delay(20);
            Release(lock);
        }
    }

}