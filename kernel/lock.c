#include <yalnix.h>
#include <ykernel.h>

#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "lock.h"
#include "bitvec.h"

/*
 * Internal struct definitions
 */
typedef struct lock {
    int lock_id;
    int lock_pid;
    struct lock *next;
    struct lock *prev;
} lock_t;

typedef struct lock_list {
    lock_t *start;
    lock_t *end;
} lock_list_t;


/*
 * Local Function Definitions
 */
static int     LockAdd(lock_list_t *_ll, lock_t *_lock);
static lock_t *LockGet(lock_list_t *_ll, int _lock_id);
static int     LockRemove(lock_list_t *_ll, int _lock_id);


/*!
 * \desc    Initializes memory for a new lock_list_t struct, which maintains a list of locks.
 *
 * \return  An initialized lock_list_t struct, NULL otherwise.
 */
lock_list_t *LockListCreate() {
    // 1. Allocate space for our lock list struct. Print message and return NULL upon error
    lock_list_t *ll = (lock_list_t *) malloc(sizeof(lock_list_t));
    if (!ll) {
        TracePrintf(1, "[LockListCreate] Error mallocing space for ll struct\n");
        return NULL;
    }

    // 2. Initialize the list start and end pointers to NULL
    ll->start     = NULL;
    ll->end       = NULL;
    return ll;
}


/*!
 * \desc           Frees the memory associated with a lock_list_t struct
 *
 * \param[in] _ll  A lock_list_t struct that the caller wishes to free
 */
int LockListDelete(lock_list_t *_ll) {
    // 1. Check arguments. Return error if invalid.
    if (!_ll) {
        TracePrintf(1, "[LockListDelete] Invalid list pointer\n");
        return ERROR;
    }

    // 2. TODO: Loop over every list and free locks. Then free lock struct
    lock_t *lock = _ll->start;
    while (lock) {
        lock_t *next = lock->next;
        free(lock);
        lock = next;
    }
    free(_ll);
    return 0;
}


/*!
 * \desc                 Creates a new lock and saves the id at the caller specified address.
 *
 * \param[in]  _ll       An initialized lock_list_t struct
 * \param[out] _lock_id  The address where the newly created lock's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int LockInit(lock_list_t *_ll, int *_lock_id) {
    // 1. Check arguments. Return ERROR if invalid.
    if (!_ll || !_lock_id) {
        TracePrintf(1, "[LockInit] One or more invalid arguments\n");
        return ERROR;
    }

    // 2. Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[LockInit] e_scheduler returned no running process\n");
        Halt();
    }

    // 3. Check that the user output variable for the lock id is within valid memory space.
    //    Specifically, every byte of the int should be in the process' region 1 memory space
    //    (i.e., in valid pages) with write permissions so we can write the lock id there.
    int ret = PTECheckAddress(running_old->pt,
                              _lock_id,
                              sizeof(int),
                              PROT_WRITE);
    if (ret < 0) {
        TracePrintf(1, "[LockInit] _lock_id pointer is not within valid address space\n");
        return ERROR;
    }

    // 4. Allocate space for a new lock struct
    lock_t *lock = (lock_t *) malloc(sizeof(lock_t));
    if (!lock) {
        TracePrintf(1, "[LockInit] Error mallocing space for lock struct\n");
        return ERROR;
    }

    // 5. Initialize internal members and increment the total number of locks
    lock->lock_id   = LockIDFindAndSet();
    if (lock->lock_id == ERROR) {
        TracePrintf(1, "[LockInit] Failed to find a valid lock_id.\n");
        free(lock);
        return ERROR;

    }
    lock->next      = NULL;
    lock->prev      = NULL;

    // 6. Add the new lock to our lock list and save the lock id in the caller's outgoing pointer
    LockAdd(_ll, lock);
    *_lock_id = lock->lock_id;

    // 7. Add the new lock id to the process's resource list
    if (list_append(running_old->res_list, lock->lock_id, NULL) == ERROR)
        return ERROR;
    return 0;
}


/*!
 * \desc                Acquires the lock for the caller. If the lock is currently taken, it will
 *                      block the caller and will not run again until a call to LockRelease moves
 *                      it back to the ready queue.
 * 
 * \param[in] _ll       An initialized lock_list_t struct
 * \param[in] _uctxt    The UserContext for the current running process
 * \param[in] _lock_id  The id of the lock that the caller wishes to acquire
 * 
 * \return              0 on success, ERROR otherwise
 */
int LockAcquire(lock_list_t *_ll, UserContext *_uctxt, int _lock_id) {
    // 1. Validate arguments.
    if (!_ll || !_uctxt) {
        TracePrintf(1, "[LockAcquire] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (!LockIDIsValid(_lock_id)) {
        TracePrintf(1, "[LockAcquire] Invalid _lock_id: %d\n", _lock_id);
        return ERROR;
    }

    // 2. Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[LockAcquire] e_scheduler returned no running process\n");
        Halt();
    }

    // 3. Grab the struct for the lock specified by lock_id. If its not found, return ERROR.
    lock_t *lock = LockGet(_ll, _lock_id);
    if (!lock) {
        TracePrintf(1, "[LockAcquire] Lock: %d not found in ll list\n", _lock_id);
        return ERROR;
    }

    // 4. Check to see if a process is currently holding the lock.
    //    If not, mark the current process as the holder and return.
    if (!lock->lock_pid) {
        lock->lock_pid = running_old->pid;
        return 0;
    }

    // 5. If a process already has the lock, then add the current process to the lock
    //    blocked list and switch to the next ready process.
    TracePrintf(1, "[LockAcquire] _lock_id: %d in use by process: %d. Blocking process: %d\n",
                                  _lock_id, lock->lock_pid, running_old->pid);
    running_old->lock_id = _lock_id;
    memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
    SchedulerAddLock(e_scheduler, running_old);
    KCSwitch(_uctxt, running_old);

    // 6. At this point, the lock should be free (check first just to be sure).
    //    So give it to the current process and return sucess.
    if (lock->lock_pid) {
        TracePrintf(1, "[LockAcquire] Error _lock_id: %d already in use by: %d\n",
                                            _lock_id, lock->lock_pid);
        Halt();
    }
    lock->lock_pid = running_old->pid;
    return 0;
}


/*!
 * \desc                Releases the lock, but only if held by the caller. If not, ERROR is
 *                      returned and the lock is not released.
 * 
 * \param[in] _ll       An initialized lock_list_t struct
 * \param[in] _lock_id  The id of the lock that the caller wishes to release
 * 
 * \return              0 on success, ERROR otherwise
 */
int LockRelease(lock_list_t *_ll, int _lock_id) {
    // 1. Validate arguments.
    if (!_ll) {
        TracePrintf(1, "[LockRelease] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (!LockIDIsValid(_lock_id)) {
        TracePrintf(1, "[LockRelease] Invalid _lock_id: %d\n", _lock_id);
        return ERROR;
    }

    // 2. Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[LockRelease] e_scheduler returned no running process\n");
        Halt();
    }

    // 3. Grab the struct for the lock specified by lock_id. If its not found, return ERROR.
    lock_t *lock = LockGet(_ll, _lock_id);
    if (!lock) {
        TracePrintf(1, "[LockRelease] Lock: %d not found in ll list\n", _lock_id);
        return ERROR;
    }

    // 4. Make sure the lock is actually being held. If not, return ERROR.
    if (!lock->lock_pid) {
        TracePrintf(1, "[LockRelease] Error _lock_id: %d is already free\n", _lock_id);
        return ERROR;
    }

    // 5. Make sure the current process actually holds the lock. If not, return ERROR.
    if (lock->lock_pid != running_old->pid) {
        TracePrintf(1, "[LockRelease] Error _lock_id: %d owned by process: %d not process: %d\n",
                                            _lock_id, lock->lock_pid, running_old->pid);
        return ERROR;
    }

    // 6. Mark the current lock as free and unblock the next process (if any) waiting on the lock
    lock->lock_pid = 0;
    SchedulerUpdateLock(e_scheduler, _lock_id);
    return 0;
}


/*!
 * \desc                Removes the lock from our lock list and frees its memory, but *only* if no
 *                      other processes are currenting waiting on it. Otherwise, we return ERROR
 *                      and do not free the lock.
 * 
 * \param[in] _ll       An initialized lock_list_t struct
 * \param[in] _lock_id  The id of the lock that the caller wishes to free
 * 
 * \return              0 on success, ERROR otherwise
 */
int LockReclaim(lock_list_t *_ll, int _lock_id) {
    // Validate arguments
    if (!_ll) helper_abort("[LockReclaim] invalid lock list pointer.\n");

    if (!LockIDIsValid(_lock_id)) {
        TracePrintf(1, "[LockReclaim] Invalid lock id %d.\n", _lock_id);
        return ERROR;
    }
    // Remove the lock from the list and free its resources
    if (LockRemove(_ll, _lock_id) == ERROR) {
        TracePrintf(1, "[LockReclaim] Failed to remove lock %d\n", _lock_id);
        Halt();
    }
    LockIDRetire(_lock_id);

    // Remove the lock id from the process's resource list
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    list_delete_key(running->res_list, _lock_id);
    return 0;
}


/*!
 * \desc             Internal function for adding a lock struct to the end of our lock list.
 * 
 * \param[in] _ll    An initialized lock_list_t struct that we wish to add the lock to
 * \param[in] _lock  The lock struct that we wish to add to the lock list
 * 
 * \return           0 on success, ERROR otherwise
 */
static int LockAdd(lock_list_t *_ll, lock_t *_lock) {
    // 1. Validate arguments
    if (!_ll || !_lock) {
        TracePrintf(1, "[LockAdd] One or more invalid argument pointers\n");
        return ERROR;
    }

    // 2. First check for our base case: the lock list is currently empty. If so,
    //    add the current lock (both as the start and end) to the lock list.
    //    Set the lock's next and previous pointers to NULL. Return success.
    if (!_ll->start) {
        _ll->start = _lock;
        _ll->end   = _lock;
        _lock->next = NULL;
        _lock->prev = NULL;
        return 0;
    }

    // 3. Our lock list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new lock as its "next" and our current lock to
    //    point to our current end as its "prev". Then, set the new lock "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    lock_t *old_end = _ll->end;
    old_end->next   = _lock;
    _lock->prev     = old_end;
    _lock->next     = NULL;
    _ll->end        = _lock;
    return 0;
}


/*!
 * \desc                Internal function for retrieving a lock struct from our lock list. Note
 *                      that this function does not modify the list---it simply returns a pointer.
 * 
 * \param[in] _ll       An initialized lock_list_t struct containing the lock we wish to retrieve
 * \param[in] _lock_id  The id of the lock that we wish to retrieve from the list
 * 
 * \return              0 on success, ERROR otherwise
 */
static lock_t *LockGet(lock_list_t *_ll, int _lock_id) {
    // 1. Loop over the lock list in search of the lock specified by _lock_id.
    //    If found, return the pointer to the lock's lock_t struct.
    lock_t *lock = _ll->start;
    while (lock) {
        if (lock->lock_id == _lock_id) {
            return lock;
        }
        lock = lock->next;
    }

    // 2. If we made it this far, then the lock specified by _lock_id was not found. Return NULL.
    TracePrintf(1, "[LockGet] Lock %d not found\n", _lock_id);
    return NULL;
}


/*!
 * \desc                Internal function for remove a lock struct from our lock list. Note that
 *                      this function does modify the list, but does not return a pointer to the
 *                      lock. Thus, the caller should use CVarGet first if they still need a
 *                      reference to the lock
 * 
 * \param[in] _ll       An initialized lock_list_t struct containing the lock we wish to remove
 * \param[in] _lock_id  The id of the lock that we wish to remove from the list
 * 
 * \return              0 on success, ERROR otherwise
 */
static int LockRemove(lock_list_t *_ll, int _lock_id) {
    // 1. Valid arguments.
    if (!_ll) {
        TracePrintf(1, "[LockRemove] Invalid pl pointer\n");
        return ERROR;
    }
    if (!_ll->start) {
        TracePrintf(1, "[LockRemove] List is empty\n");
        return ERROR;
    }
    if (!LockIDIsValid(_lock_id)) {
        TracePrintf(1, "[LockRemove] Invalid _lock_id: %d\n", _lock_id);
        return ERROR;
    }

    // 2. Check for our first base case: The lock is at the start of the list.
    lock_t *lock = _ll->start;
    if (lock->lock_id == _lock_id) {
        _ll->start = lock->next;
        if (_ll->start) {
            _ll->start->prev = NULL;
        }
        free(lock);
        return 0;
    }

    // 3. Check for our second base case: The lock is at the end of the list.
    lock = _ll->end;
    if (lock->lock_id == _lock_id) {
        _ll->end       = lock->prev;
        _ll->end->next = NULL;
        free(lock);
        return 0;
    }

    // 4. If the lock to remove is neither the first or last item in our list, then loop through
    //    the locks inbetween to it (starting from the second lock in the list). If you find the
    //    lock, remove it from the list and return success. Otherwise, return ERROR.
    lock = _ll->start->next;
    while (lock) {
        if (lock->lock_id == _lock_id) {
            lock->prev->next = lock->next;
            lock->next->prev = lock->prev;
            free(lock);
            return 0;
        }
        lock = lock->next;
    }

    // 5. If we made it this far, then the lock specified by _lock_id was not found. Return ERROR.
    TracePrintf(1, "[LockRemove] Lock %d not found\n", _lock_id);
    return ERROR;
}