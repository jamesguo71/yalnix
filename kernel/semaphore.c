#include <ykernel.h>
#include "semaphore.h"
#include "cvar.h"
#include "lock.h"
#include "kernel.h"
#include "bitvec.h"
#include "pte.h"

typedef struct sem {
  int val;
  int lock_id;
  int cvar_id;
} sem_t;

dllist *e_sem_list = NULL;

int SemInit(int *sem_idp, int val) {
    // null pointer check
    if (sem_idp == NULL) {
        TracePrintf(1, "[SyscallSemInit] error: semaphore pointer is null.\n");
        return ERROR;
    }

    // Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);

    // check if pointed-to address is in region 1 and writable
    int ret = PTECheckAddress(running_old->pt,
                              sem_idp,
                              sizeof(int),
                              PROT_WRITE);
    if (ret < 0) {
        TracePrintf(1, "[SyscallSemInit] semaphore pointer is not within valid address space\n");
        return ERROR;
    }

    // Find a free semaphore id
    int new_id = SemIDFindAndSet();
    if (new_id == ERROR) {
        TracePrintf(1, "[SyscallSemInit] Failed to find a free semaphore spot.\n");
    }
    *sem_idp = new_id;

    // Malloc a new sem_t node and set up its members
    sem_t *new_sem = malloc(sizeof(sem_t));
    if (new_sem == NULL) {
        return ERROR;
    }
    new_sem->val = val;

    ret = LockInit(e_lock_list, &new_sem->lock_id, 0);
    if (ret == ERROR) {
        free(new_sem);
        return ERROR;
    }

    ret = CVarInit(e_cvar_list, &new_sem->cvar_id, 0);
    if (ret == ERROR) {
        LockReclaim(e_lock_list, new_sem->lock_id);
        free(new_sem);
        return ERROR;
    }

    ret = list_append(e_sem_list, new_id, new_sem);
    if (ret == ERROR) {
        LockReclaim(e_lock_list, new_sem->lock_id);
        CVarReclaim(e_cvar_list, new_sem->cvar_id);
        free(new_sem);
        return ERROR;
    }

    ret = list_append(running_old->res_list, new_id, NULL);
    if (ret == ERROR) {
        LockReclaim(e_lock_list, new_sem->lock_id);
        CVarReclaim(e_cvar_list, new_sem->cvar_id);
        list_delete_key(e_sem_list, new_id);
        free(new_sem);
        return ERROR;
    }
    return SUCCESS;
}

int SemUp(UserContext *uctxt, int sem_id) {
    int ret;
    // get the corresponding semaphore from the list
    dlnode_t *node = list_find(e_sem_list, sem_id);
    if (node == NULL) {
        TracePrintf(1, "[SemUp] list_find returns error.\n");
        return ERROR;
    }
    sem_t * sem = (sem_t *) node->data;

    // acquire the lock and up the current val, then broadcast
    ret = LockAcquire(e_lock_list, uctxt, sem->lock_id);
    if (ret == ERROR) {
        TracePrintf(1, "[SemUp] LockAcquire returns error.\n");
        return ERROR;
    }
    sem->val++;
    TracePrintf(1, "[SemUp] one up the semaphore, current sem->val = %d\n", sem->val);

    ret = CVarSignal(e_cvar_list, sem->cvar_id);
    if (ret == ERROR) {
        TracePrintf(1, "[SemUp] LockRelease returns error.\n");
        return ERROR;
    }
    ret = LockRelease(e_lock_list, sem->lock_id);
    if (ret == ERROR) {
        TracePrintf(1, "[SemUp] LockRelease returns error.\n");
        return ERROR;
    }
    return SUCCESS;
}

int SemDown(UserContext *uctxt, int sem_id) {
    int ret;
    // get the corresponding semaphore from the list
    dlnode_t *node = list_find(e_sem_list, sem_id);
    if (node == NULL) {
        TracePrintf(1, "[SemDown] list_find returns error.\n");
        return ERROR;
    }
    sem_t * sem = (sem_t *) node->data;

    ret = LockAcquire(e_lock_list, uctxt, sem->lock_id);
    if (ret == ERROR) {
        TracePrintf(1, "[SemDown] LockAcquire returns error.\n");
        return ERROR;
    }
    while (sem->val == 0) {
        TracePrintf(1, "[SemDown] sem->val is 0, now wait for a SemUp.\n");
        ret = CVarWait(e_cvar_list, uctxt, sem->cvar_id, sem->lock_id);
        if (ret == ERROR) {
            TracePrintf(1, "[SemDown] CVarWait returns error.\n");
            return ERROR;
        }
    }
    sem->val--;
    TracePrintf(1, "[SemDown] Current sem->val: %d\n", sem->val);
    ret = LockRelease(e_lock_list, sem->lock_id);
    if (ret == ERROR) {
        TracePrintf(1, "[SemDown] LockRelease returns error.\n");
        return ERROR;
    }

    return SUCCESS;
}

int SemReclaim(int sem_id) {
    // Free the sem id in bitvec
    if (!SemIDIsValid(sem_id)) {
        TracePrintf(1, "[SemReclaim] Error in trying to reclaim an invalid sem_id.\n");
        return ERROR;
    }
    SemIDRetire(sem_id);

    // delete the semaphore node from the global semaphore list
    list_delete_key(e_sem_list, sem_id);

    // remove the semaphore id from the process's resource list
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    list_delete_key(running->res_list, sem_id);

    return SUCCESS;
}