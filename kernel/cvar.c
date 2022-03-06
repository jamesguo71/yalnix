#include <yalnix.h>
#include <ykernel.h>

#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "cvar.h"
#include "bitvec.h"


/*
 * Internal struct definitions
 */
typedef struct cvar {
    int cvar_id;
    struct cvar *next;
    struct cvar *prev;
} cvar_t;

typedef struct cvar_list {
    cvar_t *start;
    cvar_t *end;
} cvar_list_t;


/*
 * Local Function Definitions
 */
static int     CVarAdd(cvar_list_t *_cl, cvar_t *_cvar);
static cvar_t *CVarGet(cvar_list_t *_cl, int _cvar_id);
static int     CVarRemove(cvar_list_t *_cl, int _cvar_id);


/*!
 * \desc    Initializes memory for a new cvar_list_t struct, which maintains a list of cvars.
 *
 * \return  An initialized cvar_list_t struct, NULL otherwise.
 */
cvar_list_t *CVarListCreate() {
    // 1. Allocate space for our cvar list struct. Print message and return NULL upon error
    cvar_list_t *pl = (cvar_list_t *) malloc(sizeof(cvar_list_t));
    if (!pl) {
        TracePrintf(1, "[CVarListCreate] Error mallocing space for pl struct\n");
        return NULL;
    }

    // 2. Initialize the list start and end pointers to NULL
    pl->start     = NULL;
    pl->end       = NULL;
    return pl;
}


/*!
 * \desc           Frees the memory associated with a cvar_list_t struct
 *
 * \param[in] _cl  A cvar_list_t struct that the caller wishes to free
 */
int CVarListDelete(cvar_list_t *_cl) {
    // 1. Check arguments. Return error if invalid.
    if (!_cl) {
        TracePrintf(1, "[CVarListDelete] Invalid list pointer\n");
        return ERROR;
    }

    // 2. TODO: Loop over every list and free cvars. Then free cvar struct
    cvar_t *cvar = _cl->start;
    while (cvar) {
        cvar_t *next = cvar->next;
        free(cvar);
        cvar = next;
    }
    free(_cl);
    return 0;
}


/*!
 * \desc                 Creates a new cvar and saves the id at the caller specified address.
 *
 * \param[in]  _cl       An initialized cvar_list_t struct
 * \param[out] _cvar_id  The address where the newly created cvar's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int CVarInit(cvar_list_t *_cl, int *_cvar_id) {
    // 1. Check arguments. Return ERROR if invalid.
    if (!_cl || !_cvar_id) {
        TracePrintf(1, "[CVarInit] One or more invalid arguments\n");
        return ERROR;
    }

    // 2. Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[CVarInit] e_scheduler returned no running process\n");
        Halt();
    }

    // 3. Check that the user output variable for the cvar id is within valid memory space.
    //    Specifically, every byte of the int should be in the process' region 1 memory space
    //    (i.e., in valid pages) with write permissions so we can write the cvar id there.
    int ret = PTECheckAddress(running_old->pt,
                              _cvar_id,
                              sizeof(int),
                              PROT_WRITE);
    if (ret < 0) {
        TracePrintf(1, "[CVarInit] _cvar_id pointer is not within valid address space\n");
        return ERROR;
    }

    // 4. Allocate space for a new cvar struct
    cvar_t *cvar = (cvar_t *) malloc(sizeof(cvar_t));
    if (!cvar) {
        TracePrintf(1, "[CVarInit] Error mallocing space for cvar struct\n");
        return ERROR;
    }

    // 5. Initialize internal members and increment the total number of cvars
    cvar->cvar_id   = CVarIDFindAndSet();
    if (cvar->cvar_id == ERROR) {
        TracePrintf(1, "[CVarInit] Failed to find a valid cvar_id.\n");
        free(cvar);
        return ERROR;
    }
    cvar->next      = NULL;
    cvar->prev      = NULL;

    // 6. Add the new cvar to our cvar list and save the cvar id in the caller's outgoing pointer
    CVarAdd(_cl, cvar);
    *_cvar_id = cvar->cvar_id;

    // 7. Add the cvar to the process's resource list
    if (list_append(running_old->res_list, cvar->cvar_id) == ERROR) {
        return ERROR;
    }
    return 0;
}


/*!
 * \desc                Unblocks the next process waiting on the cvar (if any)
 * 
 * \param[in] _cl       An initialized cvar_list_t struct
 * \param[in] _cvar_id  The id of the cvar that the caller wishes to signal on
 * 
 * \return              0 on success, ERROR otherwise
 */
int CVarSignal(cvar_list_t *_cl, int _cvar_id) {
    // 1. Validate arguments.
    if (!_cl) {
        TracePrintf(1, "[CVarSignal] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (!CVarIDIsValid(_cvar_id)) {
        TracePrintf(1, "[CVarSignal] Invalid _cvar_id: %d\n", _cvar_id);
        return ERROR;
    }

    // 2. Grab the struct for the cvar specified by cvar_id. If its not found, return ERROR.
    //    Otherwise, "signal" the cvar. More specifically, check to see if there are any
    //    processes waiting on this cvar. If so, move the first to the ready queue).
    cvar_t *cvar = CVarGet(_cl, _cvar_id);
    if (!cvar) {
        TracePrintf(1, "[CVarSignal] CVar: %d not found in ll list\n", _cvar_id);
        return ERROR;
    }
    SchedulerUpdateCVar(e_scheduler, _cvar_id);
    return 0;
}


/*!
 * \desc                Unblocks *all* processes waiting on the cvar (if any)
 * 
 * \param[in] _cl       An initialized cvar_list_t struct
 * \param[in] _cvar_id  The id of the cvar that the caller wishes to signal on
 * 
 * \return              0 on success, ERROR otherwise
 */
int CVarBroadcast(cvar_list_t *_cl, int _cvar_id) {
    // 1. Validate arguments.
    if (!_cl) {
        TracePrintf(1, "[CVarBroadcast] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (!CVarIDIsValid(_cvar_id)) {
        TracePrintf(1, "[CVarBroadcast] Invalid _cvar_id: %d\n", _cvar_id);
        return ERROR;
    }

    // 2. Grab the struct for the cvar specified by cvar_id. If its not found, return ERROR.
    //    Otherwise, "signal" the cvar. More specifically, check to see if there are any
    //    processes waiting on this cvar. If so, move the first to the ready queue).
    cvar_t *cvar = CVarGet(_cl, _cvar_id);
    if (!cvar) {
        TracePrintf(1, "[CVarBroadcast] CVar: %d not found in ll list\n", _cvar_id);
        return ERROR;
    }

    // 3. I coded UpdateCVar to return ERROR if it does not find any process waiting on the
    //    cvar specified by _cvar_id. Thus, we just loop as long is it returns 0 to unblock
    //    all of the processes waiting on cvar (i.e., broadcast).
    int ret = SchedulerUpdateCVar(e_scheduler, _cvar_id);
    while (ret == 0) {
        ret = SchedulerUpdateCVar(e_scheduler, _cvar_id);
    }
    return 0;
}


/*!
 * \desc                Releases the caller held lock and adds the caller to the cvar wait list.
 *                      When the caller is unblocked (due to a signal or broadcast) we re-aquire
 *                      the lock before returning. If the caller does not hold the lock at the
 *                      time this function is called we return immediately with ERROR.
 * 
 * \param[in] _cl       An initialized cvar_list_t struct
 * \param[in] _uctxt    The UserContext for the current running process
 * \param[in] _cvar_id  The id of the cvar that the caller wishes to wait on
 * \param[in] _lock_id  The id of the lock that the caller current holds
 * 
 * \return              0 on success, ERROR otherwise
 */
int CVarWait(cvar_list_t *_cl, UserContext *_uctxt, int _cvar_id, int _lock_id) {
    // 1. Validate arguments.
    if (!_cl || !_uctxt) {
        TracePrintf(1, "[CVarWait] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (!CVarIDIsValid(_cvar_id)) {
        TracePrintf(1, "[CVarWait] Invalid _cvar_id: %d\n", _cvar_id);
        return ERROR;
    }

    // 2. Release the lock. If this fails for any reason (e.g., lock is already free or the
    //    the current process does not hold it) return an ERROR and do not continue.
    int ret = LockRelease(e_lock_list, _lock_id);
    if (ret < 0) {
        TracePrintf(1, "[CVarWait] Error releasing lock\n");
        return ERROR;
    }

    // 3. Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[CVarWait] e_scheduler returned no running process\n");
        Halt();
    }

    // 4. Mark that the current process is waiting on _cvar_id and add it to our blocked list.
    TracePrintf(1, "[CVarWait] Waiting on _cvar_id: %d for _lock_id: %d. Blocking process: %d\n",
                                  _cvar_id, _lock_id, running_old->pid);
    running_old->cvar_id = _cvar_id;
    memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
    SchedulerAddCVar(e_scheduler, running_old);
    KCSwitch(_uctxt, running_old);

    // 5. If we are here it is because the process has been woken up by a CVarSignal. Now we
    //    should re-acquire the lock before returning to normal process execution. If this
    //    fails for any reason, however, return an ERROR.
    ret = LockAcquire(e_lock_list, _uctxt, _lock_id);
    if (ret < 0) {
        TracePrintf(1, "[CVarWait] Error aquiring lock\n");
        return ERROR;
    }
    return 0;
}


/*!
 * \desc                Removes the cvar from our cvar list and frees its memory, but *only* if no
 *                      other processes are currenting waiting on it. Otherwise, we return ERROR
 *                      and do not free the cvar.
 * 
 * \param[in] _cl       A cvar_list_t struct that the caller wishes to free
 * \param[in] _cvar_id  The id of the cvar that the caller wishes to free
 * 
 * \return              0 on success, ERROR otherwise
 */
int CVarReclaim(cvar_list_t *_cl, int _cvar_id) {
    // Validate arguments
    if (!_cl) helper_abort("[CVarReclaim] invalid cvar list pointer.\n");
    if (!CVarIDIsValid(_cvar_id)) {
        TracePrintf(1, "[CVarReclaim] Invalid cvar id %d\n", _cvar_id);
        return ERROR;
    }

    // Remove the cvar from the list and free its resources
    if (CVarRemove(_cl, _cvar_id) == ERROR) {
        helper_abort("[CvarReclaim] CVar remove failed.\n");
    }
    CVarIDRetire(_cvar_id);

    // Remove the cvar from the process's resource list
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    list_delete_id(running->res_list, _cvar_id);

    return 0;
}


/*!
 * \desc             Internal function for adding a cvar struct to the end of our cvar list.
 * 
 * \param[in] _cl    An initialized cvar_list_t struct that we wish to add the cvar to
 * \param[in] _cvar  The cvar struct that we wish to add to the cvar list
 * 
 * \return           0 on success, ERROR otherwise
 */
static int CVarAdd(cvar_list_t *_cl, cvar_t *_cvar) {
    // 1. Validate arguments
    if (!_cl || !_cvar) {
        TracePrintf(1, "[CVarAdd] One or more invalid argument pointers\n");
        return ERROR;
    }

    // 2. First check for our base case: the cvar list is currently empty. If so,
    //    add the current cvar (both as the start and end) to the cvar list.
    //    Set the cvar's next and previous pointers to NULL. Return success.
    if (!_cl->start) {
        _cl->start = _cvar;
        _cl->end   = _cvar;
        _cvar->next = NULL;
        _cvar->prev = NULL;
        return 0;
    }

    // 3. Our cvar list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new cvar as its "next" and our current cvar to
    //    point to our current end as its "prev". Then, set the new cvar "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    cvar_t *old_end = _cl->end;
    old_end->next   = _cvar;
    _cvar->prev     = old_end;
    _cvar->next     = NULL;
    _cl->end       = _cvar;
    return 0;
}


/*!
 * \desc                Internal function for retrieving a cvar struct from our cvar list. Note
 *                      that this function does not modify the list---it simply returns a pointer.
 * 
 * \param[in] _cl       An initialized cvar_list_t struct containing the cvar we wish to retrieve
 * \param[in] _cvar_id  The id of the cvar that we wish to retrieve from the list
 * 
 * \return              0 on success, ERROR otherwise
 */
static cvar_t *CVarGet(cvar_list_t *_cl, int _cvar_id) {
    // 1. Loop over the cvar list in search of the cvar specified by _cvar_id.
    //    If found, return the pointer to the cvar's cvar_t struct.
    cvar_t *cvar = _cl->start;
    while (cvar) {
        if (cvar->cvar_id == _cvar_id) {
            return cvar;
        }
        cvar = cvar->next;
    }

    // 2. If we made it this far, then the cvar specified by _cvar_id was not found. Return NULL.
    TracePrintf(1, "[CVarGet] CVar %d not found\n", _cvar_id);
    return NULL;
}


/*!
 * \desc                Internal function for remove a cvar struct from our cvar list. Note that
 *                      this function does modify the list, but does not return a pointer to the
 *                      cvar. Thus, the caller should use CVarGet first if they still need a
 *                      reference to the cvar
 * 
 * \param[in] _cl       An initialized cvar_list_t struct containing the cvar we wish to remove
 * \param[in] _cvar_id  The id of the cvar that we wish to remove from the list
 * 
 * \return              0 on success, ERROR otherwise
 */
static int CVarRemove(cvar_list_t *_cl, int _cvar_id) {
    // 1. Valid arguments.
    if (!_cl) {
        TracePrintf(1, "[CVarRemove] Invalid pl pointer\n");
        return ERROR;
    }
    if (!_cl->start) {
        TracePrintf(1, "[CVarRemove] List is empty\n");
        return ERROR;
    }
    if (!CVarIDIsValid(_cvar_id)) {
        TracePrintf(1, "[CVarRemove] Invalid _cvar_id: %d\n", _cvar_id);
        return ERROR;
    }

    // 2. Check for our first base case: The cvar is at the start of the list.
    cvar_t *cvar = _cl->start;
    if (cvar->cvar_id == _cvar_id) {
        _cl->start = cvar->next;
        if (_cl->start) {
            _cl->start->prev = NULL;
        }
        free(cvar);
        return 0;
    }

    // 3. Check for our second base case: The cvar is at the end of the list.
    cvar = _cl->end;
    if (cvar->cvar_id == _cvar_id) {
        _cl->end       = cvar->prev;
        _cl->end->next = NULL;
        free(cvar);
        return 0;
    }

    // 4. If the cvar to remove is neither the first or last item in our list, then loop through
    //    the cvars inbetween to it (starting from the second cvar in the list). If you find the
    //    cvar, remove it from the list and return success. Otherwise, return ERROR.
    cvar = _cl->start->next;
    while (cvar) {
        if (cvar->cvar_id == _cvar_id) {
            cvar->prev->next = cvar->next;
            cvar->next->prev = cvar->prev;
            free(cvar);
            return 0;
        }
        cvar = cvar->next;
    }

    // 5. If we made it this far, then the cvar specified by _cvar_id was not found. Return ERROR.
    TracePrintf(1, "[CVarRemove] CVar %d not found\n", _cvar_id);
    return ERROR;
}