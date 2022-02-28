#include <yalnix.h>
#include <ykernel.h>

#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "pipe.h"


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
    int num_pipes;
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
 * \desc    Initializes memory for a new pipe_t struct
 *
 * \return  An initialized pipe_t struct, NULL otherwise.
 */
pipe_list_t *PipeListCreate() {
    // 1. Allocate space for our pipe list struct. Print message and return NULL upon error
    pipe_list_t *pl = (pipe_list_t *) malloc(sizeof(pipe_list_t));
    if (!pl) {
        TracePrintf(1, "[PipeListCreate] Error mallocing space for pl struct\n");
        return NULL;
    }

    // 2. Initialize the list start and end pointers to NULL
    pl->num_pipes = 0;
    pl->start     = NULL;
    pl->end       = NULL;
    return pl;
}


/*!
 * \desc                  Frees the memory associated with a pipe_t struct
 *
 * \param[in] _pipe  A pipe_t struct that the caller wishes to free
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
    free(pipe);
    return 0;
}


/*!
 * \desc                 Creates a new pipe.
 * 
 * \param[out] pipe_idp  The address where the newly created pipe's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int PipeInit(pipe_list_t *_pl, int *_pipe_id) {
    // 1. Check arguments. Return ERROR if invalid.
    if (!_pl || !_pipe_id) {
        TracePrintf(1, "[PipeInit] One or more invalid arguments\n");
        return ERROR;
    }

    // 2. Allocate space for a new pipe struct
    pipe_t *pipe = (pipe_t *) malloc(sizeof(pipe_t));
    if (!pipe) {
        TracePrintf(1, "[PipeInit] Error mallocing space for pipe struct\n");
        return ERROR;
    }

    // 3. Initialize internal members and increment the total number of pipes
    pipe->buf_len   = 0;
    pipe->pipe_id   = _pl->num_pipes;
    pipe->read_pid  = 0;
    pipe->write_pid = 0;
    pipe->next      = NULL;
    pipe->prev      = NULL;
    _pl->num_pipes++;

    // 4. Add the new pipe to our pipe list and save the pipe id in the caller's outgoing pointer
    PipeAdd(_pl, pipe);
    *_pipe_id = pipe->pipe_id;
    return 0;
}


/*!
 * \desc                Reads len number of bytes from the pipe indicated by pipe_id into buf.
 * 
 * \param[in]  pipe_id  The id of the pipe to read from
 * \param[out] buf      An output buffer to store the bytes read from the pipe
 * \param[in]  len      The length of the output buffer
 * 
 * \return              Number of bytes read on success, ERROR otherwise
 */
int PipeRead(pipe_list_t *_pl, UserContext *_uctxt, int _pipe_id, void *_buf, int _buf_len) {
    // 1. Validate arguments. Our pointers should not be NULL, and our pipe id and output buffer
    //    length should not be out of range. If output buffer length is 0, return 0 bytes read.
    if (!_pl || !_uctxt || !_buf) {
        TracePrintf(1, "[PipeRead] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (_pipe_id < 0 || _pipe_id >= _pl->num_pipes) {
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
    //    bytes to the beginning of the pipe buffer and update the buffer length. Additionally,
    //    unblock the next process waiting to read on the pipe (if any) if there are still bytes
    //    remaining in the pipe. Otherwise, set the pipe length to 0 since we read all of them.
    if (_buf_len < pipe->buf_len) {
        int pipe_remainder = pipe->buf_len - _buf_len;
        memcpy(pipe->buf, pipe->buf + _buf_len, pipe_remainder);
        pipe->buf_len = pipe_remainder;
        SchedulerUpdatePipeRead(e_scheduler, _pipe_id);
    } else {
        pipe->buf_len = 0;
    }

    // 9. Lastly, mark the pipe as available for reading and unblock the next process that is
    //    waiting to write to it (if any) since there is now space available in the pipe.
    pipe->read_pid = 0;
    SchedulerUpdatePipeWrite(e_scheduler, _pipe_id, pipe->write_pid);
    return read_len;
}


/*!
 * \desc               Writes the contents of the buffer to the pipe indicated by pipe_id.
 * 
 * \param[in] pipe_id  The id of the terminal to write to
 * \param[in] buf      An input buffer containing the bytes to write to the pipe
 * \param[in] len      The length of the input buffer
 * 
 * \return             Number of bytes written on success, ERROR otherwise
 */
int PipeWrite(pipe_list_t *_pl, UserContext *_uctxt, int _pipe_id, void *_buf, int _buf_len) {
    // 1. Validate arguments. Our pointers should not be NULL, and our pipe id and input buffer
    //    length should not be out of range. If input buffer length is 0, return 0 bytes written.
    if (!_pl || !_uctxt || !_buf) {
        TracePrintf(1, "[PipeWrite] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (_pipe_id < 0 || _pipe_id >= _pl->num_pipes) {
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
    int bytes_remaining = kernel_buf_len;
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
        SchedulerUpdatePipeRead(e_scheduler, _pipe_id);
        TracePrintf(1, "[PipeWrite] Process: %d wrote %d bytes to pipe: %d. Remaining bytes: %d\n",
                                    running_old->pid, pipe_remaining, _pipe_id, bytes_remaining);
        running_old->pipe_id = _pipe_id;
        pipe->write_pid      = running_old->pid;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddPipeWrite(e_scheduler, running_old);
        KCSwitch(_uctxt, running_old);
    }

    // 8. Mark the pipe as available for writing and unblock the next process (if any) that is
    //    waiting to write to, or read from, this pipe.
    pipe->write_pid = 0;
    SchedulerUpdatePipeWrite(e_scheduler, _pipe_id, pipe->write_pid);
    SchedulerUpdatePipeRead(e_scheduler, _pipe_id);
    return kernel_buf_len;
}

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
    if (_pipe_id < 0 || _pipe_id >= _pl->num_pipes) {
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