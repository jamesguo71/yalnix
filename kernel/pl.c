#include "pl.h"
#include "ykernel.h"


/*
 * Internal struct definitions
 */
typedef struct plnode {
    pcb_t *process;
    struct plnode *next;
    struct plnode *prev;
} plnode_t;


typedef struct proc_list {
    plnode_t *lists[PROC_LIST_NUM_LISTS];
} proc_list_t;


/*
 * Local Function Definitions
 */
static int    ProcListAdd(proc_list_t *_proc_list, pcb_t *_process, int _start, int _end);
static pcb_t *ProcListGet(proc_list_t *_proc_list, int _pid, int _start, int _end);
static int    ProcListPrint(proc_list_t *_proc_list, int _start);
static int    ProcListRemove(proc_list_t *_proc_list, int _pid, int _start, int _end);


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
        Halt();
    }

    // 2. Initialize the list start and end pointers to NULL
    for (int i = 0; i < PROC_LIST_NUM_LISTS; i++) {
        proc_list->lists[i] = NULL;
    }

    //
    plnode_t *running = (plnode_t *) malloc(sizeof(plnode_t));
    if (!running) {
        TracePrintf(1, "[ProcListCreate] Error mallocing space for running plnode\n");
        Halt();
    }
    running->process = NULL;
    running->next    = NULL;
    running->prev    = NULL;
    proc_list->lists[PROC_LIST_RUNNING] = running;
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
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListBlockedAdd] Invalid list or process pointer\n");
        return ERROR;
    }
    return ProcListAdd(_proc_list,
                       _process,
                       PROC_LIST_BLOCKED_START,
                       PROC_LIST_BLOCKED_END);
}


/*!
 * \desc                  F
 *
 * \param[in] _proc_list  A
 * 
 * \return                0 on success, NULL otherwise.
 */
int ProcListBlockedDelay(proc_list_t *_proc_list) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListBlockedDelay] Invalid list pointer\n");
        return ERROR;
    }

    // 2. Loop over the blocked list to check if any of the processes are blocked due to a delay
    //    call. If so, decrement their clock_ticks value then check to see if it has reached 0.
    //    If so, remove them from the blocked queue and add them to the ready queue.
    plnode_t *node = _proc_list->lists[PROC_LIST_BLOCKED_START];
    while (node) {
        if (node->process->clock_ticks) {
            node->process->clock_ticks--;
            if (node->process->clock_ticks == 0) {
                // TODO: Check these return values?
                ProcListRemove(_proc_list,
                               node->process->pid,
                               PROC_LIST_BLOCKED_START,
                               PROC_LIST_BLOCKED_END);
                ProcListAdd(_proc_list,
                            node->process,
                            PROC_LIST_READY_START,
                            PROC_LIST_READY_END);
                TracePrintf(1, "[ProcListBlockedDelay] Unblocked pid: %d\n", node->process->pid);
            }
        }
        node = node->next;
    }
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
    // 1. Check arguments and return error if invalid. Otherwise, call internal get.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListBlockedGet] Invalid list or pid\n");
        return NULL;
    }
    return ProcListGet(_proc_list,
                       _pid,
                       PROC_LIST_BLOCKED_START,
                       PROC_LIST_BLOCKED_END);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListBlockedPrint(proc_list_t *_proc_list) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListBlockedPrint] Invalid list pointer\n");
        return ERROR;
    }
    TracePrintf(1, "[ProcListBlockedPrint] Blocked List:\n");
    return ProcListPrint(_proc_list, PROC_LIST_BLOCKED_START);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListBlockedRemove(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal remove.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListBlockedRemove] Invalid list or pid\n");
        return ERROR;
    }
    return ProcListRemove(_proc_list,
                          _pid,
                          PROC_LIST_BLOCKED_START,
                          PROC_LIST_BLOCKED_END);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListProcessAdd(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListProcessAdd] Invalid list or process pointer\n");
        return ERROR;
    }
    return ProcListAdd(_proc_list,
                       _process,
                       PROC_LIST_PROCESSES_START,
                       PROC_LIST_PROCESSES_END);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
pcb_t *ProcListProcessGet(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal get.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListProcessGet] Invalid list or pid\n");
        return NULL;
    }
    return ProcListGet(_proc_list,
                       _pid,
                       PROC_LIST_PROCESSES_START,
                       PROC_LIST_PROCESSES_END);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListProcessPrint(proc_list_t *_proc_list) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListProcessPrint] Invalid list pointer\n");
        return ERROR;
    }
    TracePrintf(1, "[ProcListProcessPrint] Process List:\n");
    return ProcListPrint(_proc_list, PROC_LIST_PROCESSES_START);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListProcessRemove(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal remove.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListProcessRemove] Invalid list or pid\n");
        return ERROR;
    }
    return ProcListRemove(_proc_list,
                          _pid,
                          PROC_LIST_PROCESSES_START,
                          PROC_LIST_PROCESSES_END);
}


/*!
 * \desc                  F
 *
 * \param[in] _proc_list  A
 * 
 * \return                0 on success, NULL otherwise.
 */
int ProcListReadyAdd(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListReadyAdd] Invalid list or process pointer\n");
        return ERROR;
    }
    return ProcListAdd(_proc_list,
                       _process,
                       PROC_LIST_READY_START,
                       PROC_LIST_READY_END);
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
    if (!_proc_list->lists[PROC_LIST_READY_START]) {
        TracePrintf(1, "[ProcListReadyNext] Ready list is empty\n");
        return NULL;
    }

    // 3. Check for our base case: there is only 1 process in the ready list.
    //    If so, set the list start and end pointers to NULL and return the process.
    plnode_t *node    = _proc_list->lists[PROC_LIST_READY_START];
    pcb_t    *process = node->process;
    if (node == _proc_list->lists[PROC_LIST_READY_END]) {
        _proc_list->lists[PROC_LIST_READY_START] = NULL;
        _proc_list->lists[PROC_LIST_READY_END]   = NULL;
        free(node);
        return process;
    }

    // 4. Otherwise, update the start of the list to point to the new head (i.e., proc's
    //    next pointer). Then, clear the new head's prev pointer so it no longer points to
    //    proc and clear proc's next pointer so it no longer points to the new head.
    _proc_list->lists[PROC_LIST_READY_START]       = node->next;
    _proc_list->lists[PROC_LIST_READY_START]->prev = NULL;
    free(node);
    return process;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListReadyPrint(proc_list_t *_proc_list) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListReadyPrint] Invalid list pointer\n");
        return ERROR;
    }
    TracePrintf(1, "[ProcListReadyPrint] Ready List:\n");
    return ProcListPrint(_proc_list, PROC_LIST_READY_START);
}


/*!
 * \desc                  F
 *
 * \param[in] _proc_list  A
 * 
 * \return                0 on success, NULL otherwise.
 */
int ProcListReadyRemove(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal remove.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListReadyRemove] Invalid list or pid\n");
        return ERROR;
    }
    return ProcListRemove(_proc_list,
                          _pid,
                          PROC_LIST_READY_START,
                          PROC_LIST_READY_END);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListRunningSet(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListRunningSet] Invalid list or process pointer\n");
        return ERROR;
    }
    _proc_list->lists[PROC_LIST_RUNNING]->process = _process;
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
        return NULL;
    }
    return _proc_list->lists[PROC_LIST_RUNNING]->process;
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListTerminatedAdd(proc_list_t *_proc_list, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_proc_list || !_process) {
        TracePrintf(1, "[ProcListTerminatedAdd] Invalid list or process pointer\n");
        return ERROR;
    }
    return ProcListAdd(_proc_list,
                       _process,
                       PROC_LIST_TERMINATED_START,
                       PROC_LIST_TERMINATED_END);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
pcb_t *ProcListTerminatedGet(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal get.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListTerminatedGet] Invalid list or pid\n");
        return NULL;
    }
    return ProcListGet(_proc_list,
                       _pid,
                       PROC_LIST_TERMINATED_START,
                       PROC_LIST_TERMINATED_END);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListTerminatedPrint(proc_list_t *_proc_list) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_proc_list) {
        TracePrintf(1, "[ProcListTerminatedPrint] Invalid list pointer\n");
        return ERROR;
    }
    TracePrintf(1, "[ProcListTerminatedPrint] Terminated List:\n");
    return ProcListPrint(_proc_list, PROC_LIST_TERMINATED_START);
}


/*!
 * \desc                F
 *
 * \param[in] _process  A
 * 
 * \return              0 on success, ERROR otherwise.
 */
int ProcListTerminatedRemove(proc_list_t *_proc_list, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_proc_list || _pid < 0) {
        TracePrintf(1, "[ProcListTerminatedRemove] Invalid list or pid\n");
        return ERROR;
    }
    return ProcListRemove(_proc_list,
                          _pid,
                          PROC_LIST_TERMINATED_START,
                          PROC_LIST_TERMINATED_END);
}



static int ProcListAdd(proc_list_t *_proc_list, pcb_t *_process, int _start, int _end) {
    // 1.
    plnode_t *node = (plnode_t *) malloc(sizeof(plnode_t));
    if (!node) {
        TracePrintf(1, "[ProcListAdd] Unable to malloc memory for node\n");
        Halt();
    }
    node->process = _process;

    // 2. First check for our base case: the blocked list is currently empty. If so,
    //    add the current process (both as the start and end) to the blocked list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->lists[_start]) {
        _proc_list->lists[_start] = node;
        _proc_list->lists[_end]   = node;
        node->next = NULL;
        node->prev = NULL;
        return 0;
    }

    // 3. Our blocked list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new process as its "next" and our current process to
    //    point to our current end as its "prev". Then, set the new process "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    plnode_t *old_end            = _proc_list->lists[_end];
    old_end->next                = node;
    node->prev                   = old_end;
    node->next                   = NULL;
    _proc_list->lists[_end] = node;
    return 0;
}



static pcb_t *ProcListGet(proc_list_t *_proc_list, int _pid, int _start, int _end) {
    // 1. Loop over the terminated list in search of the process specified by _pid.
    //    If found, return the pointer to the processes pcb_t struct.
    plnode_t *node = _proc_list->lists[_start];
    while (node) {
        if (node->process->pid == _pid) {
            return node->process;
        }
        node = node->next;
    }

    // 2. If we made it this far, then the process specified by _pid was not found. Return NULL.
    TracePrintf(1, "[ProcListGet] Process %d not found\n", _pid);
    return NULL;
}



static int ProcListPrint(proc_list_t *_proc_list, int _start) {
    plnode_t *node = _proc_list->lists[_start];
    while (node) {
        TracePrintf(1, "\tpid: %d\n", node->process->pid);
        node = node->next;
    }
    return 0;
}



static int ProcListRemove(proc_list_t *_proc_list, int _pid, int _start, int _end) {
    // 1. First check for our base case: the terminated list is currently empty. If so,
    //    add the current process (both as the start and end) to the terminated list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_proc_list->lists[_start]) {
        TracePrintf(1, "[ProcListRemove] List empty\n");
        return ERROR;
    }

    // 2. Check for our first base case: The process is at the start of the terminated list.
    //    Set the new head of the list to the next terminated process pointed to by "proc".
    //    Remove the next terminated processes previous reference to "proc" as it is no
    //    longer in the list. Clear the next and prev pointers in "proc" for good measure.
    //    NOTE: Make sure you check if next is NULL so you don't try to dereference it.
    plnode_t *node = _proc_list->lists[_start];
    if (node->process->pid == _pid) {
        _proc_list->lists[_start] = node->next;
        if (_proc_list->lists[_start]) {
            _proc_list->lists[_start]->prev = NULL;
        }
        free(node);
        return 0;
    }

    // 4. Check for our second base case: The process is at the end of the terminated list.
    //    Set the new end of the list to the prev terminated process pointed to by "proc".
    //    Remove the prev terminated processes next reference to "proc" as it is no longer
    //    in the list. Clear the next and prev pointers in "proc" for good measure.
    node = _proc_list->lists[_end];
    if (node->process->pid == _pid) {
        _proc_list->lists[_end]       = node->prev;
        _proc_list->lists[_end]->next = NULL;
        free(node);
        return 0;
    }

    // 5. If the process to remove is neither the first or last item in our terminated list, then
    //    loop through the processes inbetween to locate it (starting from the second process
    //    in the list). If you find the process, remove it from the list and return success.
    node = _proc_list->lists[_start]->next;
    while (node) {
        if (node->process->pid == _pid) {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            free(node);
            return 0;
        }
        node = node->next;
    }

    // 6. If we made it this far, then the process specified by _pid was not found. Return ERROR.
    TracePrintf(1, "[ProcListRemove] Process %d not found\n", _pid);
    return ERROR;
}