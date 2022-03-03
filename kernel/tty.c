#include <ykernel.h>
#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "tty.h"


/*
 * Internal struct definitions
 */
typedef struct line {
    void *buf;
    int   buf_len;
    struct line *next;
    struct line *prev;
} line_t;

typedef struct tty {
    int     read_pid;
    int     write_pid;
    int     read_buf_len;
    line_t *read_buf_start;
    line_t *read_buf_end;
    pcb_t  *write_proc;
} tty_t;

typedef struct tty_list {
    tty_t *terminals[TTY_NUM_TERMINALS];
} tty_list_t;


/*
 * Local Function Definitions
 */
static tty_t *TTYCreate();
static int    TTYDelete(tty_t *_terminal);
static int    TTYLineAdd(tty_t *_terminal, void *_buf, int _buf_len);
static int    TTYLineRemove(tty_t *_terminal);


/*!
 * \desc    CREATE - Functions for creating tty_list_t and tty_t structs
 *
 * \return  An initialized tty_list_t or tty_t struct, NULL otherwise.
 */
tty_list_t *TTYListCreate() {
    // 1. Allocate space for our terminal struct. Print message and return NULL upon error
    tty_list_t *tty = (tty_list_t *) malloc(sizeof(tty_list_t));
    if (!tty) {
        TracePrintf(1, "[TTYListCreate] Error mallocing space for terminal struct\n");
        return NULL;
    }

    // 2. Initialize the list start and end pointers to NULL
    for (int i = 0; i < TTY_NUM_TERMINALS; i++) {
        tty->terminals[i] = TTYCreate();
        if (!tty->terminals[i]) {
            TTYListDelete(tty);
            return NULL;
        }
    }
    return tty;
}

static tty_t *TTYCreate() {
    // 1. Allocate space for a new terminal struct
    tty_t *terminal = (tty_t *) malloc(sizeof(tty_t));
    if (!terminal) {
        TracePrintf(1, "[TTYCreate] Error mallocing space for terminal struct\n");
        return NULL;
    }

    // 2. Allocate space for our TTY read and write buffers
    terminal->read_pid       = 0;
    terminal->write_pid      = 0;
    terminal->read_buf_len   = 0;
    terminal->read_buf_start = NULL;
    terminal->read_buf_end   = NULL;
    terminal->write_proc     = NULL;
    return terminal;
}


/*!
 * \desc                 DELETE - Functions for freeing tty and terminal struct memory
 *
 * \param[in] _tl       A tty_list_t struct that the caller wishes to free
 * \param[in] _terminal  A tty_t struct that the caller wishes to free
 */
int TTYListDelete(tty_list_t *_tl) {
    // 1. Check arguments. Return error if invalid.
    if (!_tl) {
        TracePrintf(1, "[TTYDelete] Invalid list pointer\n");
        return ERROR;
    }

    // 2. TODO: Loop over every list and free lines. Then free tty struct
    for (int i = 0; i < TTY_NUM_TERMINALS; i++) {
        TTYDelete(_tl->terminals[i]);
    }
    free(_tl);
    return 0;
}

static int TTYDelete(tty_t *_terminal) {
    if (!_terminal) {
        TracePrintf(1, "[TTYDelete] Terminal already deleted\n");
        return ERROR;
    }
    // TODO: Free line structs
    free(_terminal);
}


/*!
 * \desc               Reads the next line of input from the terminal indicated by tty_id.
 *
 * \param[in]  tty_id  The id of the terminal to read from
 * \param[out] buf     An output buffer to store the bytes read from the terminal
 * \param[in]  len     The length of the output buffer
 *
 * \return             Number of bytes read on success, ERROR otherwise
 */
int TTYRead(tty_list_t *_tl, UserContext *_uctxt, int _tty_id, void *_usr_read_buf, int _buf_len) {
    // 1. Validate arguments
    if (!_tl || !_uctxt || !_usr_read_buf) {
        TracePrintf(1, "[TTYRead] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (_tty_id < 0 || _tty_id > TTY_NUM_TERMINALS) {
        TracePrintf(1, "[TTYRead] Invalid tty_id: %d\n", _tty_id);
        return ERROR;
    }
    if (_buf_len <= 0) {
        TracePrintf(1, "[TTYRead] Invalid _buf_len: %d\n", _buf_len);
        return ERROR;
    }

    // 2. Get the pcb for the current running process.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[TTYRead] e_scheduler returned no running process\n");
        Halt();
    }

    // 3. Check that the user output read buffer is within valid memory space. Specifically, every
    //    byte of the buffer should be in the process' region 1 memory space (i.e., in valid pages)
    //    and have write permissions since we are supposed to write the tty data to it.
    int ret = PTECheckAddress(running_old->pt,
                              _usr_read_buf,
                              _buf_len,
                              PROT_WRITE);
    if (ret < 0) {
        TracePrintf(1, "[TTYRead] _usr_read_buf is not within valid address space\n");
        return ERROR;
    }

    // 4. Check to see if the terminal is already in use. If so, save the process' UserContext
    //    in its pcb, add it to our TTYRead blocked list, and switch to the next ready process.
    tty_t *terminal = _tl->terminals[_tty_id];
    if (terminal->read_pid) {
        TracePrintf(1, "[TTYRead] tty_id: %d already in use by process: %d. Blocking process: %d\n",
                                  _tty_id, terminal->read_pid, running_old->pid);
        running_old->tty_id = _tty_id;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddTTYRead(e_scheduler, running_old);
        KCSwitch(_uctxt, running_old);
    }

    // 5. Check to see if we already have data ready for the process to read. Note that because a
    //    user may input many lines into a terminal before a process ever bothers reading, we need
    //    to buffer all of the user's input lines. Thus, our "read_buf" is actually a linked list
    //    of line structures, where each line contains a buffer of input read from the terminal.
    //
    //    If we do not have any lines ready, mark the process as the next to read from this terminal
    //    and add it to the TTYRead blocked list. Switch to the next ready process.
    if (!terminal->read_buf_start) {
        TracePrintf(1, "[TTYRead] tty_id: %d read_buf empty. Blocking process: %d\n",
                                  _tty_id, running_old->pid);
        running_old->tty_id = _tty_id;
        terminal->read_pid  = running_old->pid;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddTTYRead(e_scheduler, running_old);
        KCSwitch(_uctxt, running_old);
    }

    // 6. At this point, the "read_buf" should be populated with input from the terminal (i.e.,
    //    there should be at least one line containing a buffer of input. Copy the line into the
    //    user's output buffer (or only part of the line depending on the user's buffer size).
    int read_len = 0;
    line_t *line = terminal->read_buf_start;
    if (_buf_len < line->buf_len) {            // if user output buffer is smaller than the next
        read_len = _buf_len;                    // line in our read buffer, than only read enough
    } else {                                    // to fill the user buffer. If the user buffer is
        read_len = line->buf_len;              // larger, then read the entire line
    }
    memcpy(_usr_read_buf, line->buf, read_len);

    // 7. Check to see if there are any remaining bytes in our line. If so, move the remaining
    //    bytes to the beginning of the line buffer and update the line buffer length.
    //    Otherwise, remove the line from our read_buf list since we've read all of its data.
    if (_buf_len < line->buf_len) {
        int line_remainder = line->buf_len - _buf_len;
        memcpy(line->buf, line->buf + _buf_len, line_remainder);
        line->buf_len = line_remainder;
    } else {
        TTYLineRemove(terminal);
    }

    // 8. Mark the terminal as available for reading and return the number of bytes read.
    terminal->read_pid = 0;
    return read_len;
}


/*!
 * \desc              Writes the contents of the buffer to the terminal indicated by tty_id.
 *
 * \param[in] tty_id  The id of the terminal to write to
 * \param[in] buf     An input buffer containing the bytes to write to the terminal
 * \param[in] len     The length of the input buffer
 *
 * \return            Number of bytes written on success, ERROR otherwise
 */
int TTYWrite(tty_list_t *_tl, UserContext *_uctxt, int _tty_id, void *_buf, int _len) {
    // Argument sanity check
    if (_tty_id < 0 || _tty_id >= NUM_TERMINALS) return ERROR;
    if (_len < 0) return ERROR;
    if (_len == 0) return 0;
    if (_len > 0 && _buf == NULL) return ERROR;
    if (_uctxt == NULL) Halt();

    // Check page validity and permission of the given _buf
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    if (PTECheckAddress(running->pt, _buf, _len, PROT_READ) < 0)
        return ERROR;

    // Update the current UserContext
    memcpy(&running->uctxt, _uctxt, sizeof(UserContext));

    // Make a copy of the _buf in the kernel
    int kernel_buf_len = _len;
    void *kernel_buf = malloc(kernel_buf_len);
    if (kernel_buf == NULL) return ERROR;
    memcpy(kernel_buf, _buf, kernel_buf_len);

    tty_t *terminal = _tl->terminals[_tty_id];
    // If there's already a processing writing, block the current one until the terminal is free
    if (terminal->write_pid) {
        running->tty_id = _tty_id;
        SchedulerAddTTYWrite(e_scheduler, running);
        KCSwitch(_uctxt, running);
    }
    // At this point, this terminal is free, so set the current process as the writer
    // terminal->write_proc = running;
    terminal->write_pid = running->pid;

    // Here we don't add the process to a blocking list, because this process has to be the first one to unblock
    // in a TrapTTYTransmit, so we first check the terminal's write_proc to see if we need to unblock a process there

    // If the writer only needs to transmit no more than TERMINAL_MAX_LINE bytes, transmit it in one go and switch out
    if (kernel_buf_len <= TERMINAL_MAX_LINE) {
        TtyTransmit(_tty_id, kernel_buf, kernel_buf_len);
        SchedulerAddTTYWrite(e_scheduler, running);
        KCSwitch(_uctxt, running);
    }
    // Otherwise, we break them up and transmit multiple times, blocking after each transmit
    else {
        for (int rem = kernel_buf_len; rem > 0; rem -= TERMINAL_MAX_LINE) {
            int offset = kernel_buf_len - rem;
            TtyTransmit(_tty_id, kernel_buf + offset, rem > TERMINAL_MAX_LINE ? TERMINAL_MAX_LINE : rem);
            SchedulerAddTTYWrite(e_scheduler, running);
            KCSwitch(_uctxt, running);
        }
    }
    // The process finished writing, clear it and unblock a waiting process if any
    terminal->write_proc = NULL;
    terminal->write_pid = 0;
    free(kernel_buf);

    terminal->write_pid = SchedulerUpdateTTYWrite(e_scheduler, _tty_id, 0);

    return kernel_buf_len;
}

// This is called in TrapTTYTransmit, and is used to add the current terminal writer to ready queue
void TTYUpdateWriter(tty_list_t *_tl, UserContext *_uctxt, int _tty_id) {
    tty_t *terminal = _tl->terminals[_tty_id];
    // Upon the TrapTTYTransmit, the terminal should be occupied by a process, so add it to ready queue
    // if (terminal->write_proc == NULL) {
    //     helper_abort("[TTYUpdateWriter] error: terminal->write_proc is NULL\n");
    // }
    terminal->write_pid = SchedulerUpdateTTYWrite(e_scheduler, _tty_id, terminal->write_pid);
}

int TTYUpdateReader(tty_list_t *_tl, int _tty_id) {
    // 1. Validate arguments
    if (!_tl) {
        TracePrintf(1, "[TTYUpdateReader] Invalid _tl pointer\n");
        return ERROR;
    }
    if (_tty_id < 0 || _tty_id > TTY_NUM_TERMINALS) {
        TracePrintf(1, "[TTYUpdateReader] Invalid tty_id: %d\n", _tty_id);
        return ERROR;
    }

    // 2. Allocate space to hold the input we are about to read from the terminal
    tty_t *terminal = _tl->terminals[_tty_id];
    void *read_buf = (void *) malloc(TERMINAL_MAX_LINE);
    if (!read_buf) {
        TracePrintf(1, "[TTYUpdateReader] Error allocating space for read buf\n");
        Halt();
    }

    // 3. Read the input from the terminal. If it returns an error, halt the machine for now.
    int read_len = TtyReceive(_tty_id,
                              read_buf,
                              TERMINAL_MAX_LINE);
    if (read_len <= 0) {
        TracePrintf(1, "[TTYUpdateReader] Error TtyReceive returned bytes: %d\n", read_len);
        Halt();
    }
    TracePrintf(1, "[TTYUpdateReader] TtyReceive returned bytes: %d\n", read_len);

    // 4. Add the newly read line to our "read_buf", which is actually a linked list of node
    //    structures (where each node contains a line of terminal input). Afterwards, check
    //    to see if we have a process waiting to read from the specified terminal. If so,
    //    remove them from the TTYRead wait list and add them to the ready list.
    TTYLineAdd(terminal, read_buf, read_len);
    SchedulerUpdateTTYRead(e_scheduler, _tty_id);
    free(read_buf);
    return 0;
}

static int TTYLineAdd(tty_t *_terminal, void *_buf, int _buf_len) {
    // 1. Validate arguments
    if (!_terminal || !_buf) {
        TracePrintf(1, "[TTYLineAdd] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (_buf_len < 0) {
        TracePrintf(1, "[TTYLineAdd] Invalid _buf_len: %d\n", _buf_len);
        return ERROR;
    }

    // 2. Allocate space for a new line node.
    line_t *line = (line_t *) malloc(sizeof(line_t));
    if (!line) {
        TracePrintf(1, "[TTYLineAdd] Error allocating space for line\n");
        Halt();
    }
    line->buf_len = _buf_len;

    // 3. Allocate space for the line and copy over the contents.
    line->buf = (void *) malloc(_buf_len);
    if (!line->buf) {
        TracePrintf(1, "[TTYLineAdd] Error allocating space for line buf\n");
        Halt();
    }
    memcpy(line->buf, _buf, _buf_len);

    // 4. First check for our base case: the read_buf list is currently empty. If so,
    //    add the current line (both as the start and end) to the read_buf list.
    //    Set the line's next and previous pointers to NULL. Return success.
    if (!_terminal->read_buf_start) {
        _terminal->read_buf_start = line;
        _terminal->read_buf_end   = line;
        line->next = NULL;
        line->prev = NULL;
        return 0;
    }

    // 5. Our read_buf list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new line as its "next" and our current line to
    //    point to our current end as its "prev". Then, set the new line "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    line_t *old_end         = _terminal->read_buf_end;
    old_end->next           = line;
    line->prev              = old_end;
    line->next              = NULL;
    _terminal->read_buf_end = line;
    return 0;
}

static int TTYLineRemove(tty_t *_terminal) {
    // 1. Check arguments. Return error if invalid.
    if (!_terminal) {
        TracePrintf(1, "[TTYLineRemove] Invalid list\n");
        return 0;
    }

    // 2. Check for our base case: there is only 1 line in the read_buf list. If so, simply
    //    set the list start and end pointers to NULL and free the line buffer and line.
    line_t *line = _terminal->read_buf_start;
    if (line == _terminal->read_buf_end) {
        _terminal->read_buf_start = NULL;
        _terminal->read_buf_end   = NULL;
        free(line->buf);
        free(line);
        return 0;
    }

    // 3. Otherwise, update the start of the list to point to the new head (i.e., lines's
    //    next pointer). Then, clear the new head's prev pointer so it no longer points to
    //    line and clear line's next pointer so it no longer points to the new head.
    _terminal->read_buf_start       = line->next;
    _terminal->read_buf_start->prev = NULL;
    free(line->buf);
    free(line);
    return 0;
}
