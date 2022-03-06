#include <yalnix.h>
#include <ykernel.h>

#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "pipe.h"
#include "bitvec.h"

/*
 * Internal struct definitions
 */
typedef struct pipe {
    char  buf[PIPE_BUFFER_LEN];
    int   buf_len;
    int   pipe_id;
    int   read_pid;
    int   write_pid;
    struct pipe *next;
    struct pipe *prev;
} pipe_t;

typedef struct pipe_list {
    pipe_t *start;
    pipe_t *end;
} pipe_list_t;


/*
 * Local Function Definitions
 */
static int     PipeAdd(pipe_list_t *_pl, pipe_t *_pipe);
static pipe_t *PipeGet(pipe_list_t *_pl, int _pipe_id);
static int     PipeRemove(pipe_list_t *_pl, int _pipe_id);


/*!
 * \desc    Initializes memory for a new pipe_list_t struct, which maintains a list of pipes.
 *
 * \return  An initialized pipe_list_t struct, NULL otherwise.
 */
pipe_list_t *PipeListCreate() {
    // 1. Allocate space for our pipe list struct. Print message and return NULL upon error
    pipe_list_t *pl = (pipe_list_t *) malloc(sizeof(pipe_list_t));
    if (!pl) {
        TracePrintf(1, "[PipeListCreate] Error mallocing space for pl struct\n");
        return NULL;
    }

    // 2. Initialize the list start and end pointers to NULL
    pl->start     = NULL;
    pl->end       = NULL;
    return pl;
}


/*!
 * \desc           Frees the memory associated with a pipe_list_t struct
 *
 * \param[in] _pl  A pipe_list_t struct that the caller wishes to free
 */
int PipeListDelete(pipe_list_t *_pl) {
    // 1. Check arguments. Return error if invalid.
    if (!_pl) {
        TracePrintf(1, "[PipeListDelete] Invalid list pointer\n");
        return ERROR;
    }

    // 2. TODO: Loop over every list and free pipes. Then free pipe struct
    pipe_t *pipe = _pl->start;
    while (pipe) {
        pipe_t *next = pipe->next;
        free(pipe);
        pipe = next;
    }
    free(_pl);
    return 0;
}


/*!
 * \desc                 Creates a new pipe and saves the id at the caller specified address.
 *
 * \param[in]  _pl       An initialized pipe_list_t struct
 * \param[out] _pipe_id  The address where the newly created pipe's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int PipeInit(pipe_list_t *_pl, int *_pipe_id) {
    // 1. Check arguments. Return ERROR if invalid.
    if (!_pl || !_pipe_id) {
        TracePrintf(1, "[PipeInit] One or more invalid arguments\n");
        return ERROR;
    }

    // 2. Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[PipeInit] e_scheduler returned no running process\n");
        Halt();
    }

    // 3. Check that the user output variable for the pipe id is within valid memory space.
    //    Specifically, every byte of the int should be in the process' region 1 memory space
    //    (i.e., in valid pages) with write permissions so we can write the pipe id there.
    int ret = PTECheckAddress(running_old->pt,
                              _pipe_id,
                              sizeof(int),
                              PROT_WRITE);
    if (ret < 0) {
        TracePrintf(1, "[PipeRead] _pipe_id pointer is not within valid address space\n");
        return ERROR;
    }

    // 4. Allocate space for a new pipe struct
    pipe_t *pipe = (pipe_t *) malloc(sizeof(pipe_t));
    if (!pipe) {
        TracePrintf(1, "[PipeInit] Error mallocing space for pipe struct\n");
        return ERROR;
    }

    // 5. Initialize internal members and increment the total number of pipes
    pipe->buf_len   = 0;
    pipe->pipe_id   = PipeIDFindAndSet();
    if (pipe->pipe_id == ERROR) {
        TracePrintf(1, "[PipeInit] Failed to find a valid pipe_id.\n");
        free(pipe);
        return ERROR;
    }
    pipe->read_pid  = 0;
    pipe->write_pid = 0;
    pipe->next      = NULL;
    pipe->prev      = NULL;

    // 6. Add the new pipe to our pipe list and save the pipe id in the caller's outgoing pointer
    PipeAdd(_pl, pipe);
    *_pipe_id = pipe->pipe_id;

    // 7. Add the pipe id to the process's resource list
    if (list_append(running_old->res_list, pipe->pipe_id) == ERROR)
        return ERROR;
    return 0;
}


/*!
 * \desc                Removes the pipe from our pipe list and frees its memory, but *only* if no
 *                      other processes are currenting waiting on it. Otherwise, we return ERROR
 *                      and do not free the pipe.
 * 
 * \param[in] _pl       An initialized pipe_list_t struct
 * \param[in] _pipe_id  The id of the pipe that the caller wishes to free
 * 
 * \return              0 on success, ERROR otherwise
 */
int PipeReclaim(pipe_list_t *_pl, int _pipe_id) {
    if (!_pl) Halt();
    if (!PipeIDIsValid(_pipe_id)) {
        TracePrintf(1, "[PipeReclaim] Error in trying to reclaim an invalid pipe id.\n");
        return ERROR;
    }
    // Remove the pipe from the list and free its resources
    if (PipeRemove(_pl, _pipe_id) == ERROR) {
        helper_abort("[PipeReclaim] error removing a pipe.\n");
    }
    PipeIDRetire(_pipe_id);

    pcb_t *running = SchedulerGetRunning(e_scheduler);
    list_delete_id(running->res_list, _pipe_id);

    return 0;
}


/*!
 * \desc                 Reads from the pipe and stores the bytes in the caller's output buffer.
 *                       The caller is not guaranteed to get _buf_len bytes back, however, if
 *                       there are not enough bytes in the pipe---we simply return whatever is
 *                       in the pipe at the time. If there are no bytes or another process is
 *                       currently reading from the pipe, however, the caller is blocked until
 *                       the pipe is free and/or has bytes to read.
 * 
 * \param[in]  _pl       An initialized pipe_list_t struct
 * \param[in]  _uctxt    The UserContext for the current running process
 * \param[in]  _pipe_id  The id of the pipe that the caller wishes to read from
 * \param[out] _buf      The output buffer for storing the bytes read from the pipe
 * \param[in]  _buf_len  The length of the output buffer
 * 
 * \return               Number of bytes read on success, ERROR otherwise
 */
int PipeRead(pipe_list_t *_pl, UserContext *_uctxt, int _pipe_id, void *_buf, int _buf_len) {
    // 1. Validate arguments. Our pointers should not be NULL, and our pipe id and output buffer
    //    length should not be out of range. If output buffer length is 0, return 0 bytes read.
    if (!_pl || !_uctxt || !_buf) {
        TracePrintf(1, "[PipeRead] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (!PipeIDIsValid(_pipe_id)) {
        TracePrintf(1, "[PipeRead] Invalid _pipe_id: %d\n", _pipe_id);
        return ERROR;
    }
    if (_buf_len < 0) {
        TracePrintf(1, "[PipeRead] Invalid buffer length: %d\n", _buf_len);
        return ERROR;
    }
    if (_buf_len == 0) {
        return 0;
    }

    // 2. Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[PipeRead] e_scheduler returned no running process\n");
        Halt();
    }

    // 3. Check that the user output read buffer is within valid memory space. Specifically, every
    //    byte of the buffer should be in the process' region 1 memory space (i.e., in valid pages)
    //    and have write permissions since we are supposed to write the pipe data to it.
    int ret = PTECheckAddress(running_old->pt,
                              _buf,
                              _buf_len,
                              PROT_WRITE);
    if (ret < 0) {
        TracePrintf(1, "[PipeRead] _buf is not within valid address space\n");
        return ERROR;
    }

    // 4. Grab the struct for the pipe specified by pipe_id. If its not found, return ERROR.
    pipe_t *pipe = PipeGet(_pl, _pipe_id);
    if (!pipe) {
        TracePrintf(1, "[PipeRead] Pipe: %d not found in pl list\n", _pipe_id);
        return ERROR;
    }

    // 5. Check to see if another process is already waiting on this pipe. If so, save the current
    //    process' UserContext, add it to our PipeRead blocked list, and switch to the next ready.
    if (pipe->read_pid) {
        TracePrintf(1, "[PipeRead] _pipe_id: %d in use by process: %d. Blocking process: %d\n",
                                      _pipe_id, pipe->read_pid, running_old->pid);
        running_old->pipe_id = _pipe_id;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddPipeRead(e_scheduler, running_old);
        KCSwitch(_uctxt, running_old);
    }

    // 6. Check to see if we already have data ready for the process to read. If we do not have any
    //    lines ready, mark the process as the next to read from this pipe and add it to the
    //    PipeRead blocked list. Switch to the next ready process.
    if (!pipe->buf_len) {
        TracePrintf(1, "[PipeRead] _pipe_id: %d buf empty. Blocking process: %d\n",
                                      _pipe_id, running_old->pid);
        running_old->pipe_id = _pipe_id;
        pipe->read_pid       = running_old->pid;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddPipeRead(e_scheduler, running_old);
        KCSwitch(_uctxt, running_old);
    }

    // 7. At this point, the pipe should be populated with input due to some other process writing
    //    to it. Copy the pipe data into the user's output buffer (or only part of the line
    //    depending on the user's buffer size).
    int read_len = 0;
    if (_buf_len < pipe->buf_len) {             // if user output buffer is smaller than the number
        read_len = _buf_len;                    // of bytes in our buffer, than only read enough
    } else {                                    // to fill the user buffer. If the user buffer is
        read_len = pipe->buf_len;               // larger, then read the entire pipe buffer
    }
    memcpy(_buf, pipe->buf, read_len);

    // 8. Check to see if there are any remaining bytes in our pipe. If so, move the remaining
    //    bytes to the beginning of the pipe buffer and update the buffer length. Otherwise,
    //    set the pipe length to 0 since we read all of them.
    if (_buf_len < pipe->buf_len) {
        int pipe_remainder = pipe->buf_len - _buf_len;
        memcpy(pipe->buf, pipe->buf + _buf_len, pipe_remainder);
        pipe->buf_len  = pipe_remainder;
    } else {
        pipe->buf_len  = 0;
    }

    // 9. Lastly, unblock the next processes that are waiting to read or write from/to the pipe.
    //    Since we just finished reading, we send SchedulerUpdatePipeRead "0" for the read_id to
    //    indicate that it should return the next waiting process (instead of a specific one).
    //    It will return the pid of the process it unblocks, or 0 if there are none.
    //
    //    For SchedulerUpdatePipeWrite, we send the write_pid of the process currently writing to
    //    to the pipe. If there is no process currently writing to the pipe (i.e., write_pid = 0)
    //    then 0 will be returned.
    pipe->read_pid  = SchedulerUpdatePipeRead(e_scheduler, _pipe_id, 0);
    pipe->write_pid = SchedulerUpdatePipeWrite(e_scheduler, _pipe_id, pipe->write_pid);
    return read_len;
}


/*!
 * \desc                Writes the bytes from the caller's input buffer into the the pipe.
 *                      Unlike read, this function guarantees to write *all* bytes from the
 *                      input buffer into the pipe, though it may require blocking a number of
 *                      times if the input buffer is (1) larger than the available space in the
 *                      pipe or (2) if others are currently writing to the pipe.
 * 
 * \param[in] _pl       An initialized pipe_list_t struct
 * \param[in] _uctxt    The UserContext for the current running process
 * \param[in] _pipe_id  The id of the pipe that the caller wishes to write to
 * \param[in] _buf      The input buffer containing bytes to write to the pipe
 * \param[in] _buf_len  The length of the input buffer
 * 
 * \return               Number of bytes read on success, ERROR otherwise
 */
int PipeWrite(pipe_list_t *_pl, UserContext *_uctxt, int _pipe_id, void *_buf, int _buf_len) {
    // 1. Validate arguments. Our pointers should not be NULL, and our pipe id and input buffer
    //    length should not be out of range. If input buffer length is 0, return 0 bytes written.
    if (!_pl || !_uctxt || !_buf) {
        TracePrintf(1, "[PipeWrite] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (!PipeIDIsValid(_pipe_id)) {
        TracePrintf(1, "[PipeWrite] Invalid _pipe_id: %d\n", _pipe_id);
        return ERROR;
    }
    if (_buf_len < 0) {
        TracePrintf(1, "[PipeWrite] Invalid buffer length: %d\n", _buf_len);
        return ERROR;
    }
    if (_buf_len == 0) {
        return 0;
    }

    // 2. Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[PipeWrite] e_scheduler returned no running process\n");
        Halt();
    }

    // 3. Check that the user output read buffer is within valid memory space. Specifically, every
    //    byte of the buffer should be in the process' region 1 memory space (i.e., in valid pages)
    //    and have write permissions since we are supposed to write the pipe data to it.
    int ret = PTECheckAddress(running_old->pt,
                              _buf,
                              _buf_len,
                              PROT_READ);
    if (ret < 0) {
        TracePrintf(1, "[PipeWrite] _buf is not within valid address space\n");
        return ERROR;
    }

    // 4. Copy the user's input buffer over into kernel space. The reason we create a copy in the
    //    kernel is that if we have to block here, its possible that another process with access
    //    to the input buffer could modify its contents (or the length) to get us to write bad
    //    data. This way, even if they do modify input buf, we will have an original copy for when
    //    we get to run again here in the kernel.
    int   kernel_buf_len = _buf_len;
    void *kernel_buf = (void *) malloc(kernel_buf_len);
    if (!kernel_buf) {
        TracePrintf(1, "[PipeWrite] Error allocating space for kernel_buf\n");
        return ERROR;
    }
    memcpy(kernel_buf, _buf, kernel_buf_len);

    // 5. Grab the struct for the pipe specified by pipe_id. If its not found, return ERROR.
    pipe_t *pipe = PipeGet(_pl, _pipe_id);
    if (!pipe) {
        TracePrintf(1, "[PipeWrite] Pipe: %d not found in pl list\n", _pipe_id);
        return ERROR;
    }

    // 5. Check to see if another process is already waiting on this pipe. If so, save the current
    //    process' UserContext, add it to our PipeWrite blocked list, and switch to the next ready.
    if (pipe->write_pid) {
        TracePrintf(1, "[PipeWrite] _pipe_id: %d in use by process: %d. Blocking process: %d\n",
                                      _pipe_id, pipe->read_pid, running_old->pid);
        running_old->pipe_id = _pipe_id;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddPipeWrite(e_scheduler, running_old);
        KCSwitch(_uctxt, running_old);
    }

    // 6. At this point, we are the only ones allowed to write to the pipe, but we also need to
    //    check if the pipe is full. If so, mark this process as the next one to write to this
    //    pipe and add it to our PipeWrite blocked list. Switch to the next ready process.
    if (pipe->buf_len == PIPE_BUFFER_LEN) {
        TracePrintf(1, "[PipeWrite] _pipe_id: %d buf full. Blocking process: %d\n",
                                      _pipe_id, running_old->pid);
        running_old->pipe_id = _pipe_id;
        pipe->write_pid      = running_old->pid;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddPipeWrite(e_scheduler, running_old);
        KCSwitch(_uctxt, running_old);
    }

    // 7. If the caller wishes to write more bytes than the pipe currently has space for, we need
    //    to loop where we (1) write as many bytes as we can (2) unblock the next process (if any)
    //    that is waiting to read the pipe and (3) block ourselves until the pipe has space again.
    int   bytes_remaining  = kernel_buf_len;
    void *kernel_buf_start = kernel_buf;
    while (bytes_remaining) {

        // 7a. Calculate how much space is left in the pipe buffer. If the number of bytes left to
        //     write can fit in the available pipe space, then simply write the bytes, update the
        //     pipe buffer length, and break the loop.
        int pipe_remaining = PIPE_BUFFER_LEN - pipe->buf_len;
        if (bytes_remaining <= pipe_remaining) {
            memcpy(pipe->buf + pipe->buf_len, kernel_buf, bytes_remaining);
            pipe->buf_len += bytes_remaining;
            break;
        }

        // 7b. If we have more remaining bytes than space in the pipe, only write as many bytes as
        //     will fit in the pipe. Update the pipe buffer length, the start of the kernel buffer,
        //     and the number of bytes remaining to be written.
        memcpy(pipe->buf + pipe->buf_len, kernel_buf, pipe_remaining);
        pipe->buf_len   += pipe_remaining;
        kernel_buf      += pipe_remaining;
        bytes_remaining -= pipe_remaining;

        // 7c. Unblock the next process (if any) that is waiting to read this pipe. Then mark
        //     ourselves as currently writing to the pipe and block until space is available.
        pipe->read_pid = SchedulerUpdatePipeRead(e_scheduler, _pipe_id, pipe->read_pid);
        TracePrintf(1, "[PipeWrite] Process: %d wrote %d bytes to pipe: %d. Remaining bytes: %d\n",
                                    running_old->pid, pipe_remaining, _pipe_id, bytes_remaining);
        running_old->pipe_id = _pipe_id;
        pipe->write_pid      = running_old->pid;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddPipeWrite(e_scheduler, running_old);
        KCSwitch(_uctxt, running_old);
    }

    // 8. Lastly, unblock the next processes that are waiting to read or write from/to the pipe.
    //    Since we just finished reading, we send SchedulerUpdatePipeWrite "0" for the write_id to
    //    indicate that it should return the next waiting process (instead of a specific one).
    //    It will return the pid of the process it unblocks, or 0 if there are none.
    //
    //    For SchedulerUpdatePipeRead, we send the read_pid of the process currently reading from
    //    the pipe. If there is no process currently reading from the pipe (i.e., read_pid = 0)
    //    then 0 will be returned.
    pipe->read_pid  = SchedulerUpdatePipeRead(e_scheduler, _pipe_id, pipe->read_pid);
    pipe->write_pid = SchedulerUpdatePipeWrite(e_scheduler, _pipe_id, 0);
    free(kernel_buf_start);
    return kernel_buf_len;
}


/*!
 * \desc             Internal function for adding a pipe struct to the end of our pipe list.
 * 
 * \param[in] _pl    An initialized lock_list_t struct that we wish to add the pipe to
 * \param[in] _pipe  The pipe struct that we wish to add to the pipe list
 * 
 * \return           0 on success, ERROR otherwise
 */
static int PipeAdd(pipe_list_t *_pl, pipe_t *_pipe) {
    // 1. Validate arguments
    if (!_pl || !_pipe) {
        TracePrintf(1, "[PipeAdd] One or more invalid argument pointers\n");
        return ERROR;
    }

    // 2. First check for our base case: the read_buf list is currently empty. If so,
    //    add the current pipe (both as the start and end) to the read_buf list.
    //    Set the pipe's next and previous pointers to NULL. Return success.
    if (!_pl->start) {
        _pl->start = _pipe;
        _pl->end   = _pipe;
        _pipe->next = NULL;
        _pipe->prev = NULL;
        return 0;
    }

    // 3. Our read_buf list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new pipe as its "next" and our current pipe to
    //    point to our current end as its "prev". Then, set the new pipe "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    pipe_t *old_end = _pl->end;
    old_end->next   = _pipe;
    _pipe->prev     = old_end;
    _pipe->next     = NULL;
    _pl->end       = _pipe;
    return 0;
}


/*!
 * \desc                Internal function for retrieving a pipe struct from our pipe list. Note
 *                      that this function does not modify the list---it simply returns a pointer.
 * 
 * \param[in] _pl       An initialized lock_list_t struct containing the pipe we wish to retrieve
 * \param[in] _pipe_id  The id of the pipe that we wish to retrieve from the list
 * 
 * \return              0 on success, ERROR otherwise
 */
static pipe_t *PipeGet(pipe_list_t *_pl, int _pipe_id) {
    // 1. Loop over the pipe list in search of the pipe specified by _pipe_id.
    //    If found, return the pointer to the pipe's pipe_t struct.
    pipe_t *pipe = _pl->start;
    while (pipe) {
        if (pipe->pipe_id == _pipe_id) {
            return pipe;
        }
        pipe = pipe->next;
    }

    // 2. If we made it this far, then the pipe specified by _pipe_id was not found. Return NULL.
    TracePrintf(1, "[PipeGet] Pipe %d not found\n", _pipe_id);
    return NULL;
}


/*!
 * \desc                Internal function for remove a pipe struct from our pipe list. Note that
 *                      this function does modify the list, but does not return a pointer to the
 *                      pipe. Thus, the caller should use CVarGet first if they still need a
 *                      reference to the pipe
 * 
 * \param[in] _pl       An initialized lock_list_t struct containing the pipe we wish to remove
 * \param[in] _pipe_id  The id of the pipe that we wish to remove from the list
 * 
 * \return              0 on success, ERROR otherwise
 */
static int PipeRemove(pipe_list_t *_pl, int _pipe_id) {
    // 1. Valid arguments.
    if (!_pl) {
        TracePrintf(1, "[PipeRemove] Invalid pl pointer\n");
        return ERROR;
    }
    if (!_pl->start) {
        TracePrintf(1, "[PipeRemove] List is empty\n");
        return ERROR;
    }
    if (!PipeIDIsValid(_pipe_id)) {
        TracePrintf(1, "[PipeRemove] Invalid _pipe_id: %d\n", _pipe_id);
        return ERROR;
    }

    // 2. Check for our first base case: The pipe is at the start of the list.
    pipe_t *pipe = _pl->start;
    if (pipe->pipe_id == _pipe_id) {
        _pl->start = pipe->next;
        if (_pl->start) {
            _pl->start->prev = NULL;
        }
        free(pipe);
        return 0;
    }

    // 3. Check for our second base case: The pipe is at the end of the list.
    pipe = _pl->end;
    if (pipe->pipe_id == _pipe_id) {
        _pl->end       = pipe->prev;
        _pl->end->next = NULL;
        free(pipe);
        return 0;
    }

    // 4. If the pipe to remove is neither the first or last item in our list, then loop through
    //    the pipes inbetween to it (starting from the second pipe in the list). If you find the
    //    pipe, remove it from the list and return success. Otherwise, return ERROR.
    pipe = _pl->start->next;
    while (pipe) {
        if (pipe->pipe_id == _pipe_id) {
            pipe->prev->next = pipe->next;
            pipe->next->prev = pipe->prev;
            free(pipe);
            return 0;
        }
        pipe = pipe->next;
    }

    // 5. If we made it this far, then the pipe specified by _pipe_id was not found. Return ERROR.
    TracePrintf(1, "[PipeRemove] Pipe %d not found\n", _pipe_id);
    return ERROR;
}