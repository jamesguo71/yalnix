#include <ykernel.h>
#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "tty.h"


/*
 * Internal struct definitions
 */
typedef struct node {
    void *line;
    int   line_len;
    struct node *next;
    struct node *prev;
} node_t;

typedef struct terminal {
    int   read_pid;
    int   write_pid;
    pcb_t *write_proc;
    int   read_buf_len;
    int   write_buf_len;
    node_t *read_buf_start;
    node_t *read_buf_end;
    void  *write_buf;
} terminal_t;

typedef struct tty {
    terminal_t *terminals[TTY_NUM_TERMINALS];
} tty_t;


/*
 * Local Function Definitions
 */
static terminal_t *TTYTerminalCreate();
static int         TTYTerminalDelete(terminal_t *_terminal);
static int         TTYTerminalLineAdd(terminal_t *_terminal, void *_line, int _line_len);
static int         TTYTerminalLineRemove(terminal_t *_terminal);


/*!
 * \desc    Initializes memory for a new tty_t struct
 *
 * \return  An initialized tty_t struct, NULL otherwise.
 */
tty_t *TTYCreate() {
    // 1. Allocate space for our terminal struct. Print message and return NULL upon error
    tty_t *tty = (tty_t *) malloc(sizeof(tty_t));
    if (!tty) {
        TracePrintf(1, "[TTYCreate] Error mallocing space for terminal struct\n");
        return NULL;
    }

    // 2. Initialize the list start and end pointers to NULL
    for (int i = 0; i < TTY_NUM_TERMINALS; i++) {
        tty->terminals[i] = TTYTerminalCreate();
        if (!tty->terminals[i]) {
            TTYDelete(tty);
            return NULL;
        }
    }
    return tty;
}

static terminal_t *TTYTerminalCreate() {
    // 1. Allocate space for a new terminal struct
    terminal_t *terminal = (terminal_t *) malloc(sizeof(terminal_t));
    if (!terminal) {
        TracePrintf(1, "[TTYTerminalCreate] Error mallocing space for terminal struct\n");
        return NULL;
    }

    // 2. Allocate space for our TTY read and write buffers
    terminal->read_buf_start = NULL;
    terminal->read_buf_end   = NULL;
    terminal->write_buf = (void *) malloc(TERMINAL_MAX_LINE);
    if (!terminal->write_buf) {
        TracePrintf(1, "[TTYTerminalCreate] Error mallocing space for terminal write_buf\n");
        free(terminal);
        return NULL;
    }

    // 3. Initialize the other internal members to 0
    terminal->read_pid      = 0;
    terminal->write_pid     = 0;
    terminal->write_proc    = NULL;
    terminal->read_buf_len  = 0;
    terminal->write_buf_len = 0;
    return terminal;
}

/*!
 * \desc                  Frees the memory associated with a tty_t struct
 *
 * \param[in] _tty  A tty_t struct that the caller wishes to free
 */
int TTYDelete(tty_t *_tty) {
    // 1. Check arguments. Return error if invalid.
    if (!_tty) {
        TracePrintf(1, "[TTYDelete] Invalid list pointer\n");
        return ERROR;
    }

    // 2. TODO: Loop over every list and free nodes. Then free tty struct
    for (int i = 0; i < TTY_NUM_TERMINALS; i++) {
        TTYTerminalDelete(_tty->terminals[i]);
    }
    free(_tty);
    return 0;
}

static int TTYTerminalDelete(terminal_t *_terminal) {
    if (!_terminal) {
        TracePrintf(1, "[TTYTerminalDelete] Terminal already deleted\n");
        return ERROR;
    }

    // if (_terminal->read_buf) {
    //     free(_terminal->read_buf);
    // }
    if (_terminal->write_buf) {
        free(_terminal->write_buf);
    }
    free(_terminal);
}

int TTYRead(tty_t *_tty, UserContext *_uctxt, int _tty_id, void *_usr_read_buf, int _buf_len) {
    // 1. Validate arguments
    if (!_tty || !_uctxt || !_usr_read_buf) {
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
    terminal_t *terminal = _tty->terminals[_tty_id];
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
    //    of node structures, where each node contains a line of input read from the terminal.
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
    //    there should be at least one node containing a line of input. Copy the line into the
    //    user's output buffer (or only part of the line depending on the user's buffer size).
    int read_len = 0;
    node_t *node = terminal->read_buf_start;
    if (_buf_len < node->line_len) {            // if user output buffer is smaller than the next
        read_len = _buf_len;                    // line in our read buffer, than only read enough
    } else {                                    // to fill the user buffer. If the user buffer is
        read_len = node->line_len;              // larger, then read the entire line
    }
    memcpy(_usr_read_buf, node->line, read_len);

    // 7. Check to see if there are any remaining bytes in our line. If so, move the remaining
    //    bytes to the beginning of the line buffer and update the line buffer length.
    //    Otherwise, remove the line from our read_buf list since we've read all of its data.
    if (_buf_len < node->line_len) {
        int line_remainder = node->line_len - _buf_len;
        memcpy(node->line, node->line + _buf_len, line_remainder);
        node->line_len = line_remainder;
    } else {
        TTYTerminalLineRemove(terminal);
    }

    // 8. Mark the terminal as available for reading and return the number of bytes read.
    terminal->read_pid = 0;
    return read_len;
}

int TTYWrite(tty_t *_tty, UserContext *_uctxt, int _tty_id, void *_buf, int _len) {
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


    // Make a copy of the _buf in the kernel
    void *kernel_buf = malloc(_len);
    if (kernel_buf == NULL) return ERROR;
    memcpy(kernel_buf, _buf, _len);

    terminal_t *terminal = _tty->terminals[_tty_id];
    // If there's already a processing writing, block the current one until the terminal is free
    if (terminal->write_proc != NULL) {
        SchedulerAddTTYWrite(e_scheduler, running);
        KCSwitch(_uctxt, running);
    }
    // At this point, this terminal is free, so set the current process as the writer
    terminal->write_proc = running;

    // Here we don't add the process to a blocking list, because this process has to be the first one to unblock
    // in a TrapTTYTransmit, so we first check the terminal's write_proc to see if we need to unblock a process there

    // If the writer only needs to transmit no more than TERMINAL_MAX_LINE bytes, transmit it in one go and switch out
    if (_len <= TERMINAL_MAX_LINE) {
        TtyTransmit(_tty_id, _buf, _len);
        KCSwitch(_uctxt, running);
    }
    // Otherwise, we break them up and transmit multiple times, blocking after each transmit
    else {
        for (int rem = _len; rem > 0; rem -= TERMINAL_MAX_LINE) {
            int offset = _len - rem;
            TtyTransmit(_tty_id, _buf + offset, rem > TERMINAL_MAX_LINE ? TERMINAL_MAX_LINE : rem);
            KCSwitch(_uctxt, running);
        }
    }
    // The process finished writing, clear it and unblock a waiting process if any
    terminal->write_proc = NULL;
    free(kernel_buf);

    SchedulerUpdateTTYWrite(e_scheduler, _tty_id);

    return _len;
}

// This is called in TrapTTYTransmit, and is used to add the current terminal writer to ready queue
void TTYUpdateWriter(tty_t *_tty, UserContext *_uctxt, int _tty_id) {
    terminal_t *terminal = _tty->terminals[_tty_id];
    // Upon the TrapTTYTransmit, the terminal should be occupied by a process, so add it to ready queue
    if (terminal->write_proc == NULL) {
        helper_abort("[TTYUpdateWriter] error: terminal->write_proc is NULL\n");
    }
    SchedulerAddReady(e_scheduler, terminal->write_proc);
}

int TTYUpdateReadBuffer(tty_t *_tty, int _tty_id) {
    // 1. Validate arguments
    if (!_tty) {
        TracePrintf(1, "[TTYUpdateReadBuffer] Invalid _tty pointer\n");
        return ERROR;
    }
    if (_tty_id < 0 || _tty_id > TTY_NUM_TERMINALS) {
        TracePrintf(1, "[TTYUpdateReadBuffer] Invalid tty_id: %d\n", _tty_id);
        return ERROR;
    }

    // 2. Allocate space to hold the input we are about to read from the terminal
    terminal_t *terminal = _tty->terminals[_tty_id];
    void *read_buf = (void *) malloc(TERMINAL_MAX_LINE);
    if (!read_buf) {
        TracePrintf(1, "[TTYUpdateReadBuffer] Error allocating space for read buf\n");
        Halt();
    }

    // 3. Read the input from the terminal. If it returns an error, halt the machine for now.
    int read_len = TtyReceive(_tty_id,
                              read_buf,
                              TERMINAL_MAX_LINE);
    if (read_len <= 0) {
        TracePrintf(1, "[TTYUpdateReadBuffer] Error TtyReceive returned bytes: %d\n", read_len);
        Halt();
    }
    TracePrintf(1, "[TTYUpdateReadBuffer] TtyReceive returned bytes: %d\n", read_len);

    // 4. Add the newly read line to our "read_buf", which is actually a linked list of node
    //    structures (where each node contains a line of terminal input). Afterwards, check
    //    to see if we have a process waiting to read from the specified terminal. If so,
    //    remove them from the TTYRead wait list and add them to the ready list.
    TTYTerminalLineAdd(terminal, read_buf, read_len);
    SchedulerUpdateTTYRead(e_scheduler, _tty_id);
    free(read_buf);
    return 0;
}

static int TTYTerminalLineAdd(terminal_t *_terminal, void *_line, int _line_len) {
    // 1. Validate arguments
    if (!_terminal || !_line) {
        TracePrintf(1, "[TTYTerminalLineAdd] One or more invalid argument pointers\n");
        return ERROR;
    }
    if (_line_len < 0) {
        TracePrintf(1, "[TTYTerminalLineAdd] Invalid _line_len: %d\n", _line_len);
        return ERROR;
    }

    // 2. Allocate space for a new line node.
    node_t *node = (node_t *) malloc(sizeof(node_t));
    if (!node) {
        TracePrintf(1, "[TTYTerminalLineAdd] Error allocating space for node\n");
        Halt();
    }
    node->line_len = _line_len;

    // 3. Allocate space for the line and copy over the contents.
    node->line = (void *) malloc(_line_len);
    if (!node->line) {
        TracePrintf(1, "[TTYTerminalLineAdd] Error allocating space for line buf\n");
        Halt();
    }
    memcpy(node->line, _line, _line_len);

    // 4. First check for our base case: the read_buf list is currently empty. If so,
    //    add the current line (both as the start and end) to the read_buf list.
    //    Set the line's next and previous pointers to NULL. Return success.
    if (!_terminal->read_buf_start) {
        _terminal->read_buf_start = node;
        _terminal->read_buf_end   = node;
        node->next = NULL;
        node->prev = NULL;
        return 0;
    }

    // 5. Our read_buf list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new line as its "next" and our current line to
    //    point to our current end as its "prev". Then, set the new line "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    node_t *old_end         = _terminal->read_buf_end;
    old_end->next           = node;
    node->prev              = old_end;
    node->next              = NULL;
    _terminal->read_buf_end = node;
    return 0;
}

static int TTYTerminalLineRemove(terminal_t *_terminal) {
    // 1. Check arguments. Return error if invalid.
    if (!_terminal) {
        TracePrintf(1, "[TTYTerminalLineRemove] Invalid list\n");
        return 0;
    }

    // 2. Check for our base case: there is only 1 line in the read_buf list. If so, simply
    //    set the list start and end pointers to NULL and free the line buffer and node.
    node_t *node = _terminal->read_buf_start;
    if (node == _terminal->read_buf_end) {
        _terminal->read_buf_start = NULL;
        _terminal->read_buf_end   = NULL;
        free(node->line);
        free(node);
        return 0;
    }

    // 3. Otherwise, update the start of the list to point to the new head (i.e., lines's
    //    next pointer). Then, clear the new head's prev pointer so it no longer points to
    //    line and clear line's next pointer so it no longer points to the new head.
    _terminal->read_buf_start       = node->next;
    _terminal->read_buf_start->prev = NULL;
    free(node->line);
    free(node);
    return 0;
}
