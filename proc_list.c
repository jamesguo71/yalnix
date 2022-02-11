#include "proc_list.h"
#include "ykernel.h"


/*
 * Internal struct definitions
 */
typedef struct proc_list {
    pcb_t *blocked_start;
    pcb_t *blocked_end;
    pcb_t *ready_start;
    pcb_t *ready_end;
    pcb_t *terminated_start;
    pcb_t *terminated_end;
    pcb_t *running;
    pcb_t *processes_start;
    pcb_t *processes_end;
} proc_list_t;


/*!
 * \desc    Initializes memory for a new proc_list_t struct
 *
 * \return  An initialized proc_list_t struct, NULL otherwise.
 */
proc_list_t *ProcListCreate() {
    // 1. Allocate space for our process struct. Print message and return NULL upon error
    proc_list_t *proc_list = (proc_list_t *) malloc(sizeof(proc_list_t));
    if (!proc_list) {
        TracePrintf(1, "[ProcListCreate] Error mallocing space for process struct\n");
        return NULL;
    }

    // 2. Initialize the list start and end pointers to NULL
    proc_list->blocked_start    = NULL;
    proc_list->blocked_end      = NULL;
    proc_list->ready_start      = NULL;
    proc_list->ready_end        = NULL;
    proc_list->terminated_start = NULL;
    proc_list->terminated_end   = NULL;
    proc_list->running          = NULL;
    proc_list->processes_start  = NULL;
    proc_list->processes_end    = NULL;
    return proc_list;
}


/*!
 * \desc                  Frees the memory associated with a proc_list_t struct
 *
 * \param[in] _proc_list  A proc_list_t struct that the caller wishes to free
 */
int ProcListDelete(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListDelete] Invalid list pointer\n");
        return ERROR;
    }

    // TODO: Should I delete all of the pcbs too? or just the proc_list struct
    free(_proc_list);
    return 0;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListBlockedAdd(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListBlockedAdd] Invalid list or process pointer\n");
        return ERROR;
    }

    // 2. First check for our base case: the blocked list is currently empty. If so,
    //    add the current process (both as the start and end) to the blocked list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->blocked_start) {
        _proc_list->blocked_start = _process;
        _proc_list->blocked_end   = _process;
        _process->blocked_next    = NULL;
        _process->blocked_prev    = NULL;
        return 0;
    }

    // 3. Our blocked list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new process as its "next" and our current process to
    //    point to our current end as its "prev". Then, set the new process "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    pcb_t *old_end          = _proc_list->blocked_end;
    old_end->blocked_next   = _process;
    _process->blocked_prev  = old_end;
    _process->blocked_next  = NULL;
    _proc_list->blocked_end = _process;
    return 0;
}


/*!
 * \desc                  F
 *
 * \param[in] _proc_list  A
 * 
 * \return                0 on success, NULL otherwise.
 */
pcb_t *ProcListBlockedGet(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListBlockedGet] Invalid list or pid\n");
        return NULL;
    }

    // 2. Loop over the blocked list in search of the process specified by _pid.
    //    If found, return the pointer to the processes pcb_t struct.
    for (pcb_t *proc = _proc_list->blocked_start; proc != NULL; proc = proc->blocked_next) {
        if (proc->pid == _pid) {
            return proc;
        }
    }

    // 3. If we made it this far, then the process specified by _pid was not found. Return NULL.
    TracePrintf(1, "[ProcListBlockedGet] Process %d not found in blocked list\n", _pid);
    return NULL;
}


/*!
 * \desc                  F
 *
 * \param[in] _proc_list  A
 * 
 * \return                0 on success, NULL otherwise.
 */
pcb_t *ProcListBlockedNext(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListBlockedNext] Invalid list pointer\n");
        return NULL;
    }

    // 2. Check to see that we actually have processes in our blocked list. If not, return NULL
    if (!_proc_list->blocked_start) {
        TracePrintf(1, "[ProcListBlockedNext] Blocked list is empty\n");
        return NULL;
    }

    // 3. Check for our base case: there is only 1 process in the blocked list.
    //    If so, set the list start and end pointers to NULL and return the process.
    pcb_t *proc = _proc_list->blocked_start;
    if (proc == _proc_list->blocked_end) {
        _proc_list->blocked_start = NULL;
        _proc_list->blocked_end   = NULL;
        return proc;
    }

    // 4. Otherwise, update the start of the list to point to the new head (i.e., proc's
    //    next pointer). Then, clear the new head's prev pointer so it no longer points to
    //    proc and clear proc's next pointer so it no longer points to the new head.
    _proc_list->blocked_start               = proc->blocked_next;
    _proc_list->blocked_start->blocked_prev = NULL;
    proc->blocked_next                      = NULL;
    return proc;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListBlockedPrint(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListBlockedPrint] Invalid list pointer\n");
        return ERROR;
    }

    // 2.
    TracePrintf(1, "[ProcListBlockedPrint] Blocked List (pids):       [");
    pcb_t *proc = _proc_list->blocked_start;
    while (proc) {
        TracePrintf(1, "%d", proc->pid);
        proc = proc->blocked_next;
        if (proc) {
            TracePrintf(1, ", ");
        }
    }
    TracePrintf(1, "]\n");
    return 0;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListBlockedRemove(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListBlockedRemove] Invalid list or pid\n");
        return ERROR;
    }

    // 2. First check for our base case: the blocked list is currently empty. If so,
    //    add the current process (both as the start and end) to the blocked list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->blocked_start) {
        TracePrintf(1, "[ProcListBlockedRemove] Blocked list empty\n");
        return ERROR;
    }

    // 3. Check for our first base case: The process is at the start of the blocked list.
    //    Set the new head of the list to the next blocked process pointed to by "proc".
    //    Remove the next blocked processes previous reference to "proc" as it is no
    //    longer in the list. Clear the next and prev pointers in "proc" for good measure.
    pcb_t *proc = _proc_list->blocked_start;
    if (proc->pid == _pid) {
        _proc_list->blocked_start               = proc->blocked_next;
        _proc_list->blocked_start->blocked_prev = NULL;
        proc->blocked_next                      = NULL;
        proc->blocked_prev                      = NULL;
        return 0;
    }

    // 4. Check for our second base case: The process is at the end of the blocked list.
    //    Set the new end of the list to the prev blocked process pointed to by "proc".
    //    Remove the prev blocked processes next reference to "proc" as it is no longer
    //    in the list. Clear the next and prev pointers in "proc" for good measure.
    proc = _proc_list->blocked_end;
    if (proc->pid == _pid) {
        _proc_list->blocked_end               = proc->blocked_prev;
        _proc_list->blocked_end->blocked_next = NULL;
        proc->blocked_next                    = NULL;
        proc->blocked_prev                    = NULL;
        return 0;
    }

    // 5. If the process to remove is neither the first or last item in our blocked list, then
    //    loop through the processes inbetween to locate it (starting from the second process
    //    in the list). If you find the process, remove it from the list and return success.
    for (proc = _proc_list->blocked_start->blocked_next; proc != NULL; proc = proc->blocked_next) {
        if (proc->pid == _pid) {
            proc->blocked_prev->blocked_next = proc->blocked_next;
            proc->blocked_next->blocked_prev = proc->blocked_prev;
            return 0;
        }
    }

    // 6. If we made it this far, then the process specified by _pid was not found. Return ERROR.
    TracePrintf(1, "[ProcListBlockedRemove] Process %d not found in blocked list\n", _pid);
    return ERROR;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListProcessAdd(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListProcessAdd] Invalid list or process pointer\n");
        return ERROR;
    }

    // 2. First check for our base case: the process list is currently empty. If so,
    //    add the current process (both as the start and end) to the process list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->processes_start) {
        _proc_list->processes_start = _process;
        _proc_list->processes_end   = _process;
        _process->processes_next    = NULL;
        _process->processes_prev    = NULL;
        return 0;
    }

    // 3. Our process list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new process as its "next" and our current process to
    //    point to our current end as its "prev". Then, set the new process "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    pcb_t *old_end            = _proc_list->processes_end;
    old_end->processes_next   = _process;
    _process->processes_prev  = old_end;
    _process->processes_next  = NULL;
    _proc_list->processes_end = _process;
    return 0;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
pcb_t *ProcListProcessGet(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListProcessGet] Invalid list or pid\n");
        return NULL;
    }

    // 2. Loop over the process list in search of the process specified by _pid.
    //    If found, return the pointer to the processes pcb_t struct.
    for (pcb_t *proc = _proc_list->processes_start; proc != NULL; proc = proc->processes_next) {
        if (proc->pid == _pid) {
            return proc;
        }
    }

    // 3. If we made it this far, then the process specified by _pid was not found. Return NULL.
    TracePrintf(1, "[ProcListProcessGet] Process %d not found in process list\n", _pid);
    return NULL;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListProcessPrint(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListProcessPrint] Invalid list pointer\n");
        return ERROR;
    }

    // 2.
    TracePrintf(1, "[ProcListProcessPrint] Process List (pids):       [");
    pcb_t *proc = _proc_list->processes_start;
    while (proc) {
        TracePrintf(1, "%d", proc->pid);
        proc = proc->processes_next;
        if (proc) {
            TracePrintf(1, ", ");
        }
    }
    TracePrintf(1, "]\n");
    return 0;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListProcessRemove(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListProcessRemove] Invalid list or pid\n");
        return ERROR;
    }

    // 2. First check for our base case: the process list is currently empty. If so,
    //    add the current process (both as the start and end) to the process list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->processes_start) {
        TracePrintf(1, "[ProcListProcessRemove] Process list empty\n");
        return ERROR;
    }

    // 3. Check for our first base case: The process is at the start of the process list.
    //    Set the new head of the list to the next process process pointed to by "proc".
    //    Remove the next process processes previous reference to "proc" as it is no
    //    longer in the list. Clear the next and prev pointers in "proc" for good measure.
    pcb_t *proc = _proc_list->processes_start;
    if (proc->pid == _pid) {
        _proc_list->processes_start                 = proc->processes_next;
        _proc_list->processes_start->processes_prev = NULL;
        proc->processes_next                        = NULL;
        proc->processes_prev                        = NULL;
        return 0;
    }

    // 4. Check for our second base case: The process is at the end of the process list.
    //    Set the new end of the list to the prev process process pointed to by "proc".
    //    Remove the prev process processes next reference to "proc" as it is no longer
    //    in the list. Clear the next and prev pointers in "proc" for good measure.
    proc = _proc_list->processes_end;
    if (proc->pid == _pid) {
        _proc_list->processes_end                 = proc->processes_prev;
        _proc_list->processes_end->processes_next = NULL;
        proc->processes_next                      = NULL;
        proc->processes_prev                      = NULL;
        return 0;
    }

    // 5. If the process to remove is neither the first or last item in our process list, then
    //    loop through the processes inbetween to locate it (starting from the second process
    //    in the list). If you find the process, remove it from the list and return success.
    for (proc = _proc_list->processes_start->processes_next; proc != NULL; proc = proc->processes_next) {
        if (proc->pid == _pid) {
            proc->processes_prev->processes_next = proc->processes_next;
            proc->processes_next->processes_prev = proc->processes_prev;
            return 0;
        }
    }

    // 6. If we made it this far, then the process specified by _pid was not found. Return ERROR.
    TracePrintf(1, "[ProcListProcessRemove] Process %d not found in process list\n", _pid);
    return ERROR;
}


/*!
 * \desc                  F
 *
 * \param[in] _proc_list  A
 * 
 * \return                0 on success, NULL otherwise.
 */
int ProcListReadyAdd(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListReadyAdd] Invalid list or process pointer\n");
        return ERROR;
    }

    // 2. First check for our base case: the ready list is currently empty. If so,
    //    add the current process (both as the start and end) to the ready list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->ready_start) {
        _proc_list->ready_start = _process;
        _proc_list->ready_end   = _process;
        _process->ready_next    = NULL;
        _process->ready_prev    = NULL;
        return 0;
    }

    // 3. Our ready list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new process as its "next" and our current process to
    //    point to our current end as its "prev". Then, set the new process "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    pcb_t *old_end        = _proc_list->ready_end;
    old_end->ready_next   = _process;
    _process->ready_prev  = old_end;
    _process->ready_next  = NULL;
    _proc_list->ready_end = _process;
    return 0;
}


/*!
 * \desc                  F
 *
 * \param[in] _proc_list  A
 * 
 * \return                0 on success, NULL otherwise.
 */
pcb_t *ProcListReadyGet(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListReadyGet] Invalid list or pid\n");
        return NULL;
    }

    // 2. Loop over the ready list in search of the process specified by _pid.
    //    If found, return the pointer to the processes pcb_t struct.
    for (pcb_t *proc = _proc_list->ready_start; proc != NULL; proc = proc->ready_next) {
        if (proc->pid == _pid) {
            return proc;
        }
    }

    // 3. If we made it this far, then the process specified by _pid was not found. Return NULL.
    TracePrintf(1, "[ProcListReadyGet] Process %d not found in ready list\n", _pid);
    return NULL;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
pcb_t *ProcListReadyNext(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListReadyNext] Invalid list\n");
        return NULL;
    }

    // 2. Check to see that we actually have processes in our ready list. If not, return NULL
    if (!_proc_list->ready_start) {
        TracePrintf(1, "[ProcListReadyNext] Ready list is empty\n");
        return NULL;
    }

    // 3. Check for our base case: there is only 1 process in the ready list.
    //    If so, set the list start and end pointers to NULL and return the process.
    pcb_t *proc = _proc_list->ready_start;
    if (proc == _proc_list->ready_end) {
        _proc_list->ready_start = NULL;
        _proc_list->ready_end   = NULL;
        return proc;
    }

    // 4. Otherwise, update the start of the list to point to the new head (i.e., proc's
    //    next pointer). Then, clear the new head's prev pointer so it no longer points to
    //    proc and clear proc's next pointer so it no longer points to the new head.
    _proc_list->ready_start               = proc->ready_next;
    _proc_list->ready_start->ready_prev   = NULL;
    proc->ready_next                      = NULL;
    return proc;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListReadyPrint(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListReadyPrint] Invalid list pointer\n");
        return ERROR;
    }

    // 2.
    TracePrintf(1, "[ProcListReadyPrint] Ready List (pids):           [");
    pcb_t *proc = _proc_list->ready_start;
    while (proc) {
        TracePrintf(1, "%d", proc->pid);
        proc = proc->ready_next;
        if (proc) {
            TracePrintf(1, ", ");
        }
    }
    TracePrintf(1, "]\n");
    return 0;
}


/*!
 * \desc                  F
 *
 * \param[in] _proc_list  A
 * 
 * \return                0 on success, NULL otherwise.
 */
int ProcListReadyRemove(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListReadyRemove] Invalid list or pid\n");
        return ERROR;
    }

    // 2. First check for our base case: the ready list is currently empty. If so,
    //    add the current process (both as the start and end) to the ready list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->ready_start) {
        TracePrintf(1, "[ProcListReadyRemove] Ready list is empty\n");
        return ERROR;
    }

    // 3. Check for our first base case: The process is at the start of the ready list.
    //    Set the new head of the list to the next ready process pointed to by "proc".
    //    Remove the next ready processes previous reference to "proc" as it is no
    //    longer in the list. Clear the next and prev pointers in "proc" for good measure.
    pcb_t *proc = _proc_list->ready_start;
    if (proc->pid == _pid) {
        _proc_list->ready_start             = proc->ready_next;
        _proc_list->ready_start->ready_prev = NULL;
        proc->ready_next                    = NULL;
        proc->ready_prev                    = NULL;
        return 0;
    }

    // 4. Check for our second base case: The process is at the end of the ready list.
    //    Set the new end of the list to the prev ready process pointed to by "proc".
    //    Remove the prev ready processes next reference to "proc" as it is no longer
    //    in the list. Clear the next and prev pointers in "proc" for good measure.
    proc = _proc_list->ready_end;
    if (proc->pid == _pid) {
        _proc_list->ready_end             = proc->ready_prev;
        _proc_list->ready_end->ready_next = NULL;
        proc->ready_next                  = NULL;
        proc->ready_prev                  = NULL;
        return 0;
    }

    // 5. If the process to remove is neither the first or last item in our ready list, then
    //    loop through the processes inbetween to locate it (starting from the second process
    //    in the list). If you find the process, remove it from the list and return success.
    for (proc = _proc_list->ready_start->ready_next; proc != NULL; proc = proc->ready_next) {
        if (proc->pid == _pid) {
            proc->ready_prev->ready_next = proc->ready_next;
            proc->ready_next->ready_prev = proc->ready_prev;
            return 0;
        }
    }

    // 6. If we made it this far, then the process specified by _pid was not found. Return ERROR.
    TracePrintf(1, "[ProcListReadyRemove] Process %d not found in ready list\n", _pid);
    return ERROR;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListRunningAdd(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListRunningAdd] Invalid list or process pointer\n");
        return ERROR;
    }

    // 2. Print a warning in case there is already another process running.
    //    Then, set the incoming process as the new running process.
    if (_proc_list->running) {
        TracePrintf(1, "[ProcListRunningAdd] Replacing process %d\n", _proc_list->running->pid);
    }
    _proc_list->running = _process;
    return 0;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
pcb_t *ProcListRunningGet(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListRunningGet] Invalid list pointer\n");
        return ERROR;
    }
    return _proc_list->running;
}

/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListRunningRemove(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListRunningRemove] Invalid list pointer\n");
        return ERROR;
    }

    // 2. Print a warning in case there is not a process running.
    //    Then, set the running process pointer to NULL.
    if (!_proc_list->running) {
        TracePrintf(1, "[ProcListRunningRemove] There is no running process\n");
        return ERROR;
    }
    _proc_list->running = NULL;
    return 0;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
pcb_t *ProcListRunningSwitch(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListRunningSwitch] Invalid list or process pointer\n");
        return ERROR;
    }

    // 2.
    pcb_t *old_running  = _proc_list->running;
    _proc_list->running = _process;
    return old_running;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListTerminatedAdd(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListTerminatedAdd] Invalid list or process pointer\n");
        return ERROR;
    }

    // 2. First check for our base case: the terminated list is currently empty. If so,
    //    add the current process (both as the start and end) to the terminated list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->terminated_start) {
        _proc_list->terminated_start = _process;
        _proc_list->terminated_end   = _process;
        _process->terminated_next    = NULL;
        _process->terminated_prev    = NULL;
        return 0;
    }

    // 3. Our terminated list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new process as its "next" and our current process to
    //    point to our current end as its "prev". Then, set the new process "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    pcb_t *old_end             = _proc_list->terminated_end;
    old_end->terminated_next   = _process;
    _process->terminated_prev  = old_end;
    _process->terminated_next  = NULL;
    _proc_list->terminated_end = _process;
    return 0;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
pcb_t *ProcListTerminatedGet(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListTerminatedGet] Invalid list or pid\n");
        return NULL;
    }

    // 2. Loop over the terminated list in search of the process specified by _pid.
    //    If found, return the pointer to the processes pcb_t struct.
    for (pcb_t *proc = _proc_list->terminated_start; proc != NULL; proc = proc->terminated_next) {
        if (proc->pid == _pid) {
            return proc;
        }
    }

    // 3. If we made it this far, then the process specified by _pid was not found. Return NULL.
    TracePrintf(1, "[ProcListTerminatedGet] Process %d not found in terminated list\n", _pid);
    return NULL;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListTerminatedPrint(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListTerminatedPrint] Invalid list pointer\n");
        return ERROR;
    }

    // 2.
    TracePrintf(1, "[ProcListTerminatedPrint] Terminated List (pids): [");
    pcb_t *proc = _proc_list->terminated_start;
    while (proc) {
        TracePrintf(1, "%d", proc->pid);
        proc = proc->terminated_next;
        if (proc) {
            TracePrintf(1, ", ");
        }
    }
    TracePrintf(1, "]\n");
    return 0;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListTerminatedRemove(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListTerminatedRemove] Invalid list or pid\n");
        return ERROR;
    }

    // 2. First check for our base case: the terminated list is currently empty. If so,
    //    add the current process (both as the start and end) to the terminated list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->blocked_start) {
        TracePrintf(1, "[ProcListTerminatedRemove] Blocked list empty\n");
        return ERROR;
    }

    // 3. Check for our first base case: The process is at the start of the terminated list.
    //    Set the new head of the list to the next terminated process pointed to by "proc".
    //    Remove the next terminated processes previous reference to "proc" as it is no
    //    longer in the list. Clear the next and prev pointers in "proc" for good measure.
    pcb_t *proc = _proc_list->terminated_start;
    if (proc->pid == _pid) {
        proc_list->terminated_start               = proc->terminated_next;
        proc_list->terminated_start->blocked_prev = NULL;
        proc->terminated_next                     = NULL;
        proc->terminated_prev                     = NULL;
        return 0;
    }

    // 4. Check for our second base case: The process is at the end of the terminated list.
    //    Set the new end of the list to the prev terminated process pointed to by "proc".
    //    Remove the prev terminated processes next reference to "proc" as it is no longer
    //    in the list. Clear the next and prev pointers in "proc" for good measure.
    proc = _proc_list->terminated_end;
    if (proc->pid == _pid) {
        proc_list->terminated_end                  = proc->terminated_prev;
        proc_list->terminated_end->terminated_next = NULL;
        proc->terminated_next                      = NULL;
        proc->terminated_prev                      = NULL;
        return 0;
    }

    // 5. If the process to remove is neither the first or last item in our terminated list, then
    //    loop through the processes inbetween to locate it (starting from the second process
    //    in the list). If you find the process, remove it from the list and return success.
    for (proc = _proc_list->terminated_start->terminated_next; proc != NULL; proc = proc->terminated_next) {
        if (proc->pid == _pid) {
            proc->terminated_prev->terminated_next = proc->terminated_next;
            proc->terminated_next->terminated_prev = proc->terminated_prev;
            return 0;
        }
    }

    // 6. If we made it this far, then the process specified by _pid was not found. Return ERROR.
    TracePrintf(1, "[ProcListTerminatedRemove] Process %d not found in terminated list\n", _pid);
    return ERROR;
}