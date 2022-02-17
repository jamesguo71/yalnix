#include "scheduler.h"
#include "ykernel.h"


/*
 * Internal struct definitions
 */
typedef struct node {
    pcb_t *process;
    struct node *next;
    struct node *prev;
} node_t;

typedef struct scheduler {
    node_t *lists[SCHEDULER_NUM_LISTS];
} scheduler_t;


/*
 * Local Function Definitions
 */
static int    SchedulerAdd(scheduler_t *_scheduler, pcb_t *_process, int _start, int _end);
static pcb_t *SchedulerGet(scheduler_t *_scheduler, int _pid, int _start, int _end);
static int    SchedulerPrint(scheduler_t *_scheduler, int _start);
static int    SchedulerRemove(scheduler_t *_scheduler, int _pid, int _start, int _end);


/*!
 * \desc    Initializes memory for a new scheduler_t struct
 *
 * \return  An initialized scheduler_t struct, NULL otherwise.
 */
scheduler_t *SchedulerCreate() {
    // 1. Allocate space for our process struct. Print message and return NULL upon error
    scheduler_t *scheduler = (scheduler_t *) malloc(sizeof(scheduler_t));
    if (!scheduler) {
        TracePrintf(1, "[SchedulerCreate] Error mallocing space for process struct\n");
        Halt();
    }

    // 2. Initialize the list start and end pointers to NULL
    for (int i = 0; i < SCHEDULER_NUM_LISTS; i++) {
        scheduler->lists[i] = NULL;
    }

    // 3. Go ahead and initialize a node for the running process since we will only ever need
    //    one and we never delete the node. Set its internel pointers to NULL though.
    node_t *running = (node_t *) malloc(sizeof(node_t));
    if (!running) {
        TracePrintf(1, "[SchedulerCreate] Error mallocing space for running node\n");
        Halt();
    }
    running->process = NULL;
    running->next    = NULL;
    running->prev    = NULL;
    scheduler->lists[SCHEDULER_RUNNING] = running;
    return scheduler;
}


/*!
 * \desc                  Frees the memory associated with a scheduler_t struct
 *
 * \param[in] _scheduler  A scheduler_t struct that the caller wishes to free
 */
int SchedulerDelete(scheduler_t *_scheduler) {
    // 1. Check arguments. Return error if invalid.
    if (!_scheduler) {
        TracePrintf(1, "[SchedulerDelete] Invalid list pointer\n");
        return ERROR;
    }

    // 2. TODO: Loop over every list and free nodes. Then free scheduler struct
    free(_scheduler);
    return 0;
}


/*!
 * \desc                  ADD FUNCTIONS - These functions are used to add processes to one of the 
 *                        scheduler's internal lists. Aside from AddRunning, they are all thin
 *                        wrappers that call our internal add function. Since the running "list"
 *                        is only ever 1 node long, we simply update the existing node's process
 *                        pointer instead of calling add and appending a completely new node.
 *
 * \param[in] _scheduler  An initialized scheduler_t struct
 * \param[in] _process    The PCB of the process the caller wishes to add to the specified list
 * \param[in] _start      The index of the specified list's start node (only used internally)
 * \param[in] _end        The index of the specified list's end node   (only used internally)
 * 
 * \return                0 on success, ERROR otherwise.
 */
int SchedulerAddDelay(scheduler_t *_scheduler, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_scheduler || !_process) {
        TracePrintf(1, "[SchedulerAddDelay] Invalid list or process pointer\n");
        return ERROR;
    }
    return SchedulerAdd(_scheduler,
                       _process,
                       SCHEDULER_DELAY_START,
                       SCHEDULER_DELAY_END);
}

int SchedulerAddLock(scheduler_t *_scheduler, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_scheduler || !_process) {
        TracePrintf(1, "[SchedulerAddLock] Invalid list or process pointer\n");
        return ERROR;
    }
    return SchedulerAdd(_scheduler,
                       _process,
                       SCHEDULER_LOCK_START,
                       SCHEDULER_LOCK_END);
}

int SchedulerAddPipe(scheduler_t *_scheduler, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_scheduler || !_process) {
        TracePrintf(1, "[SchedulerAddPipe] Invalid list or process pointer\n");
        return ERROR;
    }
    return SchedulerAdd(_scheduler,
                       _process,
                       SCHEDULER_PIPE_START,
                       SCHEDULER_PIPE_END);
}

int SchedulerAddProcess(scheduler_t *_scheduler, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_scheduler || !_process) {
        TracePrintf(1, "[SchedulerAddProcess] Invalid list or process pointer\n");
        return ERROR;
    }
    return SchedulerAdd(_scheduler,
                       _process,
                       SCHEDULER_PROCESSES_START,
                       SCHEDULER_PROCESSES_END);
}

int SchedulerAddReady(scheduler_t *_scheduler, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_scheduler || !_process) {
        TracePrintf(1, "[SchedulerAddReady] Invalid list or process pointer\n");
        return ERROR;
    }
    return SchedulerAdd(_scheduler,
                       _process,
                       SCHEDULER_READY_START,
                       SCHEDULER_READY_END);
}

int SchedulerAddRunning(scheduler_t *_scheduler, pcb_t *_process) {
    // 1. Check arguments. Return error if invalid. Otherwise, switch the current running process.
    if (!_scheduler || !_process) {
        TracePrintf(1, "[SchedulerAddRunning] Invalid list or process pointer\n");
        return ERROR;
    }
    _scheduler->lists[SCHEDULER_RUNNING]->process = _process;
    return 0;
}

int SchedulerAddTerminated(scheduler_t *_scheduler, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_scheduler || !_process) {
        TracePrintf(1, "[SchedulerAddTerminated] Invalid list or process pointer\n");
        return ERROR;
    }
    return SchedulerAdd(_scheduler,
                       _process,
                       SCHEDULER_TERMINATED_START,
                       SCHEDULER_TERMINATED_END);
}

int SchedulerAddTTYRead(scheduler_t *_scheduler, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_scheduler || !_process) {
        TracePrintf(1, "[SchedulerAddTTYRead] Invalid list or process pointer\n");
        return ERROR;
    }
    return SchedulerAdd(_scheduler,
                       _process,
                       SCHEDULER_TTY_READ_START,
                       SCHEDULER_TTY_READ_END);
}

int SchedulerAddTTYWrite(scheduler_t *_scheduler, pcb_t *_process) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal add.
    if (!_scheduler || !_process) {
        TracePrintf(1, "[SchedulerAddTTYWrite] Invalid list or process pointer\n");
        return ERROR;
    }
    return SchedulerAdd(_scheduler,
                       _process,
                       SCHEDULER_TTY_WRITE_START,
                       SCHEDULER_TTY_WRITE_END);
}

static int SchedulerAdd(scheduler_t *_scheduler, pcb_t *_process, int _start, int _end) {
    // 1.
    node_t *node = (node_t *) malloc(sizeof(node_t));
    if (!node) {
        TracePrintf(1, "[SchedulerAdd] Unable to malloc memory for node\n");
        Halt();
    }
    node->process = _process;

    // 2. First check for our base case: the blocked list is currently empty. If so,
    //    add the current process (both as the start and end) to the blocked list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_scheduler->lists[_start]) {
        _scheduler->lists[_start] = node;
        _scheduler->lists[_end]   = node;
        node->next = NULL;
        node->prev = NULL;
        return 0;
    }

    // 3. Our blocked list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new process as its "next" and our current process to
    //    point to our current end as its "prev". Then, set the new process "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    node_t *old_end         = _scheduler->lists[_end];
    old_end->next           = node;
    node->prev              = old_end;
    node->next              = NULL;
    _scheduler->lists[_end] = node;
    return 0;
}


/*!
 * \desc                  GET FUNCTIONS - These functions are used to get the pcb associated with
 *                        a pid in one of the specified lists. Note that GetReady and GetRunning
 *                        act differently than the other basic wrappers.
 * 
 *                        GetReady removes and returns the pcb of the process at the beginning of
 *                        the list (i.e., it returns the next process that should be run).
 * 
 *                        GetRunning returns the one (and only) pcb in its "list", which is the pcb
 *                        of the process that is currently running.
 * 
 *                        All of the other "getter" functions will loop over their specified list
 *                        and return the pcb associated with the given pid (if the pcb is in the
 *                        list). Unlike GetReady, these getters do not modify their lists.
 *
 * \param[in] _scheduler  An initialized scheduler_t struct
 * \param[in] _pid        The pid of the process the caller wishes to retrieve from the specified list
 * \param[in] _start      The index of the specified list's start node (only used internally)
 * \param[in] _end        The index of the specified list's end node   (only used internally)
 * 
 * \return                PCB of the process associated with pid on success, NULL otherwise.
 */
pcb_t *SchedulerGetDelay(scheduler_t *_scheduler, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal get.
    if (!_scheduler || _pid < 0) {
        TracePrintf(1, "[SchedulerGetDelay] Invalid list or pid\n");
        return NULL;
    }
    return SchedulerGet(_scheduler,
                       _pid,
                       SCHEDULER_DELAY_START,
                       SCHEDULER_DELAY_END);
}

pcb_t *SchedulerGetProcess(scheduler_t *_scheduler, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal get.
    if (!_scheduler || _pid < 0) {
        TracePrintf(1, "[SchedulerGetProcess] Invalid list or pid\n");
        return NULL;
    }
    return SchedulerGet(_scheduler,
                       _pid,
                       SCHEDULER_PROCESSES_START,
                       SCHEDULER_PROCESSES_END);
}

pcb_t *SchedulerGetReady(scheduler_t *_scheduler) {
    // 1. Check arguments. Return error if invalid.
    if (!_scheduler) {
        TracePrintf(1, "[SchedulerGetReady] Invalid list\n");
        return NULL;
    }

    // 2. Check to see that we actually have processes in our ready list. If not, return NULL
    if (!_scheduler->lists[SCHEDULER_READY_START]) {
        TracePrintf(1, "[SchedulerGetReady] Ready list is empty\n");
        return NULL;
    }

    // 3. Check for our base case: there is only 1 process in the ready list.
    //    If so, set the list start and end pointers to NULL and return the process.
    node_t *node    = _scheduler->lists[SCHEDULER_READY_START];
    pcb_t    *process = node->process;
    if (node == _scheduler->lists[SCHEDULER_READY_END]) {
        _scheduler->lists[SCHEDULER_READY_START] = NULL;
        _scheduler->lists[SCHEDULER_READY_END]   = NULL;
        free(node);
        return process;
    }

    // 4. Otherwise, update the start of the list to point to the new head (i.e., proc's
    //    next pointer). Then, clear the new head's prev pointer so it no longer points to
    //    proc and clear proc's next pointer so it no longer points to the new head.
    _scheduler->lists[SCHEDULER_READY_START]       = node->next;
    _scheduler->lists[SCHEDULER_READY_START]->prev = NULL;
    free(node);
    return process;
}

pcb_t *SchedulerGetRunning(scheduler_t *_scheduler) {
    // 1. Check arguments. Return error if invalid.
    if (!_scheduler) {
        TracePrintf(1, "[SchedulerGetRunning] Invalid list pointer\n");
        return NULL;
    }
    return _scheduler->lists[SCHEDULER_RUNNING]->process;
}

pcb_t *SchedulerGetTerminated(scheduler_t *_scheduler, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal get.
    if (!_scheduler || _pid < 0) {
        TracePrintf(1, "[SchedulerGetTerminated] Invalid list or pid\n");
        return NULL;
    }
    return SchedulerGet(_scheduler,
                       _pid,
                       SCHEDULER_TERMINATED_START,
                       SCHEDULER_TERMINATED_END);
}

static pcb_t *SchedulerGet(scheduler_t *_scheduler, int _pid, int _start, int _end) {
    // 1. Loop over the terminated list in search of the process specified by _pid.
    //    If found, return the pointer to the processes pcb_t struct.
    node_t *node = _scheduler->lists[_start];
    while (node) {
        if (node->process->pid == _pid) {
            return node->process;
        }
        node = node->next;
    }

    // 2. If we made it this far, then the process specified by _pid was not found. Return NULL.
    TracePrintf(1, "[SchedulerGet] Process %d not found\n", _pid);
    return NULL;
}


/*!
 * \desc                  PRINT FUNCTIONS - These functions loop over the specified list and print
 *                        the pids of the processes contained within.
 *
 * \param[in] _scheduler  An initialized scheduler_t struct
 * \param[in] _start      The index of the specified list's start node (only used internally)
 * 
 * \return                0 on success, ERROR otherwise.
 */
int SchedulerPrintDelay(scheduler_t *_scheduler) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_scheduler) {
        TracePrintf(1, "[SchedulerPrintDelay] Invalid list pointer\n");
        return ERROR;
    }
    TracePrintf(1, "[SchedulerPrintDelay] Blocked List:\n");
    return SchedulerPrint(_scheduler, SCHEDULER_DELAY_START);
}

int SchedulerPrintProcess(scheduler_t *_scheduler) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_scheduler) {
        TracePrintf(1, "[SchedulerPrintProcess] Invalid list pointer\n");
        return ERROR;
    }
    TracePrintf(1, "[SchedulerPrintProcess] Process List:\n");
    return SchedulerPrint(_scheduler, SCHEDULER_PROCESSES_START);
}

int SchedulerPrintReady(scheduler_t *_scheduler) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_scheduler) {
        TracePrintf(1, "[SchedulerPrintReady] Invalid list pointer\n");
        return ERROR;
    }
    TracePrintf(1, "[SchedulerPrintReady] Ready List:\n");
    return SchedulerPrint(_scheduler, SCHEDULER_READY_START);
}

int SchedulerPrintTerminated(scheduler_t *_scheduler) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_scheduler) {
        TracePrintf(1, "[SchedulerPrintTerminated] Invalid list pointer\n");
        return ERROR;
    }
    TracePrintf(1, "[SchedulerPrintTerminated] Terminated List:\n");
    return SchedulerPrint(_scheduler, SCHEDULER_TERMINATED_START);
}

static int SchedulerPrint(scheduler_t *_scheduler, int _start) {
    node_t *node = _scheduler->lists[_start];
    while (node) {
        TracePrintf(1, "\tpid: %d\n", node->process->pid);
        node = node->next;
    }
    return 0;
}


/*!
 * \desc                  REMOVE FUNCTIONS - These functions are used to remove the pcb associated
 *                        with pid from the specified list. Note that these do NOT return the pcb
 *                        of the removed process, so the caller should ensure they already have the
 *                        pcb pointer (via "Get" or some other method).
 *
 * \param[in] _scheduler  An initialized scheduler_t struct
 * \param[in] _pid        The pid of the process the caller wishes to remove from the specified list
 * \param[in] _start      The index of the specified list's start node (only used internally)
 * \param[in] _end        The index of the specified list's end node   (only used internally)
 * 
 * \return                0 on success, ERROR otherwise.
 */
int SchedulerRemoveDelay(scheduler_t *_scheduler, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal remove.
    if (!_scheduler || _pid < 0) {
        TracePrintf(1, "[SchedulerRemoveDelay] Invalid list or pid\n");
        return ERROR;
    }
    return SchedulerRemove(_scheduler,
                          _pid,
                          SCHEDULER_DELAY_START,
                          SCHEDULER_DELAY_END);
}

int SchedulerRemoveProcess(scheduler_t *_scheduler, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal remove.
    if (!_scheduler || _pid < 0) {
        TracePrintf(1, "[SchedulerRemoveProcess] Invalid list or pid\n");
        return ERROR;
    }
    return SchedulerRemove(_scheduler,
                          _pid,
                          SCHEDULER_PROCESSES_START,
                          SCHEDULER_PROCESSES_END);
}

int SchedulerRemoveReady(scheduler_t *_scheduler, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal remove.
    if (!_scheduler || _pid < 0) {
        TracePrintf(1, "[SchedulerRemoveReady] Invalid list or pid\n");
        return ERROR;
    }
    return SchedulerRemove(_scheduler,
                          _pid,
                          SCHEDULER_READY_START,
                          SCHEDULER_READY_END);
}

int SchedulerRemoveTerminated(scheduler_t *_scheduler, int _pid) {
    // 1. Check arguments and return error if invalid. Otherwise, call internal print.
    if (!_scheduler || _pid < 0) {
        TracePrintf(1, "[SchedulerRemoveTerminated] Invalid list or pid\n");
        return ERROR;
    }
    return SchedulerRemove(_scheduler,
                          _pid,
                          SCHEDULER_TERMINATED_START,
                          SCHEDULER_TERMINATED_END);
}

static int SchedulerRemove(scheduler_t *_scheduler, int _pid, int _start, int _end) {
    // 1. First check for our base case: the terminated list is currently empty. If so,
    //    add the current process (both as the start and end) to the terminated list.
    //    Set the process' next and previous pointers to NULL. Return success.
    if (!_scheduler->lists[_start]) {
        TracePrintf(1, "[SchedulerRemove] List empty\n");
        return ERROR;
    }

    // 2. Check for our first base case: The process is at the start of the terminated list.
    //    Set the new head of the list to the next terminated process pointed to by "proc".
    //    Remove the next terminated processes previous reference to "proc" as it is no
    //    longer in the list. Clear the next and prev pointers in "proc" for good measure.
    //    NOTE: Make sure you check if next is NULL so you don't try to dereference it.
    node_t *node = _scheduler->lists[_start];
    if (node->process->pid == _pid) {
        _scheduler->lists[_start] = node->next;
        if (_scheduler->lists[_start]) {
            _scheduler->lists[_start]->prev = NULL;
        }
        free(node);
        return 0;
    }

    // 4. Check for our second base case: The process is at the end of the terminated list.
    //    Set the new end of the list to the prev terminated process pointed to by "proc".
    //    Remove the prev terminated processes next reference to "proc" as it is no longer
    //    in the list. Clear the next and prev pointers in "proc" for good measure.
    node = _scheduler->lists[_end];
    if (node->process->pid == _pid) {
        _scheduler->lists[_end]       = node->prev;
        _scheduler->lists[_end]->next = NULL;
        free(node);
        return 0;
    }

    // 5. If the process to remove is neither the first or last item in our terminated list, then
    //    loop through the processes inbetween to locate it (starting from the second process
    //    in the list). If you find the process, remove it from the list and return success.
    node = _scheduler->lists[_start]->next;
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
    TracePrintf(1, "[SchedulerRemove] Process %d not found\n", _pid);
    return ERROR;
}


/*!
 * \desc                  UPDATE FUNCTIONS - These functions are used to update the state of the
 *                        "blocked" lists (e.g., delay, lock, pipe, tty). They loop over the
 *                        specified list to check if any of the processes meet the necessary
 *                        conditions for being unblocked. If so, they are removed from the list
 *                        and added to the ready list.
 *
 * \param[in] _scheduler  An initialized scheduler_t struct
 * 
 * \return                0 on success, ERROR otherwise.
 */
int SchedulerUpdateDelay(scheduler_t *_scheduler) {
    // 1. Check arguments. Return error if invalid.
    if (!_scheduler) {
        TracePrintf(1, "[SchedulerUpdateDelay] Invalid list pointer\n");
        return ERROR;
    }

    // 2. Loop over the blocked list to check if any of the processes are blocked due to a delay
    //    call. If so, decrement their clock_ticks value then check to see if it has reached 0.
    //    If so, remove them from the blocked queue and add them to the ready queue.
    node_t *node = _scheduler->lists[SCHEDULER_DELAY_START];
    while (node) {
        if (node->process->clock_ticks) {
            node->process->clock_ticks--;
            if (node->process->clock_ticks == 0) {
                // TODO: Check these return values?
                SchedulerRemove(_scheduler,
                               node->process->pid,
                               SCHEDULER_DELAY_START,
                               SCHEDULER_DELAY_END);
                SchedulerAdd(_scheduler,
                            node->process,
                            SCHEDULER_READY_START,
                            SCHEDULER_READY_END);
                TracePrintf(1, "[SchedulerUpdateDelay] Unblocked pid: %d\n", node->process->pid);
            }
        }
        node = node->next;
    }
    return 0;
}

int SchedulerUpdateLock(scheduler_t *_scheduler) {
    return 0;
}

int SchedulerUpdatePipe(scheduler_t *_scheduler) {
    return 0;
}

int SchedulerUpdateTTYRead(scheduler_t *_scheduler) {
    return 0;
}

int SchedulerUpdateTTYWrite(scheduler_t *_scheduler) {
    return 0;
}