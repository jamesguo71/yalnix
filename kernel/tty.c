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
static int         TTYLineAdd(terminal_t *_terminal, void *_line, int _line_len);
static int         TTYLineRemove(terminal_t *_terminal);


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
        SchedulerPrintTTYRead(e_scheduler);
        KCSwitch(_uctxt, running_old);
    }

    // 5. Check to see if we already have data ready for the process to read.
    //    If not, mark the process as the next to read and block it.
    //
    //    TODO: Add TTYGetReadLine?
    if (!terminal->read_buf_start) {
        TracePrintf(1, "[TTYRead] tty_id: %d read_buf empty. Blocking process: %d\n",
                                  _tty_id, running_old->pid);
        running_old->tty_id = _tty_id;
        terminal->read_pid  = running_old->pid;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddTTYRead(e_scheduler, running_old);
        SchedulerPrintTTYRead(e_scheduler);
        KCSwitch(_uctxt, running_old);
    }

    // 6. At this point, the read_buf should be populated with data. Write the tty
    //    data into the user's output buffer, but first figure out how much to read.
    int read_len = 0;
    node_t *line = terminal->read_buf_start;
    if (_buf_len < line->line_len) {            // if user output buffer is smaller than the amount
        read_len = _buf_len;                    // of data in our tty read buffer, than only read
    } else {                                    // enough to fill the user buffer. If the output
        read_len = line->line_len;              // buffer is larger, then read all of the bytes in
    }                                           // the tty read buffer.
    memcpy(_usr_read_buf, line->line, read_len);

    // 7. Check to see if there are any remaining bytes in our tty read buffer. If so, move the
    //    remaining bytes to the beginning of the read buffer and update the read buffer length.
    //    Otherwise, set the read buffer length to 0 since we read all of the data.
    if (_buf_len < line->line_len) {
        int read_remainder = line->line_len - _buf_len;
        memcpy(line->line, line->line + _buf_len, read_remainder);
        line->line_len = read_remainder;
    } else {
        TTYLineRemove(terminal);
    }

    // 8. Mark the terminal as available for reading and return the number of bytes read.
    terminal->read_pid = 0;
    return read_len;
}

int TTYWrite(tty_t *_tty, UserContext *_uctxt, int _tty_id, void *_usr_write_buf, int _buf_len) {
    // 1. Validate arguments

    // 2. Check that the write buff is pointing to valid memory

    // 3. Check to see if the terminal is already in use. If so, block the current process
    // if (_tty->terminals[_tty_id]->write_remaining) {
    //     // block and context switch here
    // }

    // 
    return 0;
}

int TTYUpdateReadBuffer(tty_t *_tty, int _tty_id) {
    // 1. Validate arguments

    // 2. Check to see if we already have data in our terminal's read buffer. If so, increment the
    //    receive_count variable to indicate that the hardware has more data for us. Then return.
    terminal_t *terminal = _tty->terminals[_tty_id];
    void *read_buf = (void *) malloc(TERMINAL_MAX_LINE);
    if (!read_buf) {
        TracePrintf(1, "[TTYUpdateReadBuffer] Error allocating space for read buf\n");
        Halt();
    }

    // 3. If our terminal's read buffer is empty, then go ahead and read from the terminal and
    //    store its output in our read buffer. Update its length to the number of bytes read.
    int read_len = TtyReceive(_tty_id,
                              read_buf,
                              TERMINAL_MAX_LINE);
    if (read_len <= 0) {
        TracePrintf(1, "[TTYUpdateReadBuffer] Error TtyReceive returned bytes: %d\n", read_len);
        Halt();
    }
    TracePrintf(1, "[TTYUpdateReadBuffer] TtyReceive returned bytes: %d\n", read_len);

    // 
    TTYLineAdd(terminal, read_buf, read_len);
    free(read_buf);

    // 4. Check to see if we have a process waiting to read from the specified terminal.
    //    If so, remove them from the TTYRead wait list and add them to the ready list.
    SchedulerUpdateTTYRead(e_scheduler, _tty_id);
    SchedulerPrintTTYRead(e_scheduler);
    return 0;
}

static int TTYLineAdd(terminal_t *_terminal, void *_line, int _line_len) {
    // 1. Validate arguments

    // 2.
    node_t *node = (node_t *) malloc(sizeof(node_t));
    if (!node) {
        TracePrintf(1, "[TTYLineAdd] Error allocating space for node\n");
        Halt();
    }

    // 3.
    node->line = (void *) malloc(_line_len);
    if (!node->line) {
        TracePrintf(1, "[TTYLineAdd] Error allocating space for line buf\n");
        Halt();        
    }
    node->line_len = _line_len;

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

    // 5. Our blocked list is not empty. Our list is doubly linked, so we need to set the
    //    current end to point to our new process as its "next" and our current process to
    //    point to our current end as its "prev". Then, set the new process "next" to NULL
    //    since it is the end of the list and update our end-of-list pointer in the list struct.
    node_t *old_end         = _terminal->read_buf_end;
    old_end->next           = node;
    node->prev              = old_end;
    node->next              = NULL;
    _terminal->read_buf_end = node;
    return 0;
}

static int TTYLineRemove(terminal_t *_terminal) {
    // 1. Check arguments. Return error if invalid.
    if (!_terminal) {
        TracePrintf(1, "[TTYLineRemove] Invalid list\n");
        return 0;
    }

    // 3. Check for our base case: there is only 1 process in the ready list.
    //    If so, set the list start and end pointers to NULL and return the process.
    node_t *node = _terminal->read_buf_start;
    if (node == _terminal->read_buf_end) {
        _terminal->read_buf_start = NULL;
        _terminal->read_buf_end   = NULL;
        free(node->line);
        free(node);
        return 0;
    }

    // 4. Otherwise, update the start of the list to point to the new head (i.e., proc's
    //    next pointer). Then, clear the new head's prev pointer so it no longer points to
    //    proc and clear proc's next pointer so it no longer points to the new head.
    _terminal->read_buf_start       = node->next;
    _terminal->read_buf_start->prev = NULL;
    free(node->line);
    free(node);
    return 0;
}
