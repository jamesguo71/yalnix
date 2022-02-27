#include <ykernel.h>
#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "tty.h"


/*
 * Internal struct definitions
 */
typedef struct terminal {
    int   receive_count;
    int   read_pid;
    int   write_pid;
    int   read_buf_len;
    int   write_buf_len;
    void  *read_buf;
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
    terminal->read_buf = (void *) malloc(TERMINAL_MAX_LINE);
    if (!terminal->read_buf) {
        TracePrintf(1, "[TTYTerminalCreate] Error mallocing space for terminal read_buf\n");
        free(terminal);
        return NULL;
    }
    terminal->write_buf = (void *) malloc(TERMINAL_MAX_LINE);
    if (!terminal->write_buf) {
        TracePrintf(1, "[TTYTerminalCreate] Error mallocing space for terminal write_buf\n");
        free(terminal->read_buf);
        free(terminal);
        return NULL;
    }

    // 3. Initialize the other internal members to 0
    terminal->receive_count = 0;
    terminal->read_pid      = 0;
    terminal->write_pid     = 0;
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

    if (_terminal->read_buf) {
        free(_terminal->read_buf);
    }
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

    // 5. Check to see if we already have data stored in our terminal read buffer. If not,
    //    check to see if the hardware has data ready for us to read from the terminal.
    if (!terminal->read_buf_len) {

        // 5a. We don't have data in our read buffer *and* the hardware does not have any data
        //     ready at the terminal. So, mark the current process as the next one that should
        //     get to read from this terminal and block it until the read buf is populated.
        if (!terminal->receive_count) {
            TracePrintf(1, "[TTYRead] tty_id: %d read_buf empty. Blocking process: %d\n",
                                      _tty_id, running_old->pid);
            running_old->tty_id = _tty_id;
            terminal->read_pid  = running_old->pid;
            memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
            SchedulerAddTTYRead(e_scheduler, running_old);
            SchedulerPrintTTYRead(e_scheduler);
            KCSwitch(_uctxt, running_old);
        } 

        // 5b. We don't have data in our read buffer *but* the hardware does have data ready at the
        //     terminal. Go ahead and read up to TERMINAL_MAX_LINE bytes into our terminal's read
        //     buffer and decrement our receive counter. Do not block the process; instead, the
        //     process should continue on to read the bytes from the newly populated read buff.
        else {
            TTYUpdateReadBuffer(_tty, _tty_id);
            terminal->receive_count--;
        }
    }

    // 6. At this point, the read_buf should be populated with data. Write the tty
    //    data into the user's output buffer, but first figure out how much to read.
    int read_len = 0;
    if (_buf_len < terminal->read_buf_len) {    // if user output buffer is smaller than the amount
        read_len = _buf_len;                    // of data in our tty read buffer, than only read
    } else {                                    // enough to fill the user buffer. If the output
        read_len = terminal->read_buf_len;      // buffer is larger, then read all of the bytes in
    }                                           // the tty read buffer.
    memcpy(_usr_read_buf, terminal->read_buf, read_len);

    // 7. Check to see if there are any remaining bytes in our tty read buffer. If so, move the
    //    remaining bytes to the beginning of the read buffer and update the read buffer length.
    //    Otherwise, set the read buffer length to 0 since we read all of the data.
    if (_buf_len < terminal->read_buf_len) {
        int read_remainder = terminal->read_buf_len - _buf_len;
        memcpy(terminal->read_buf, terminal->read_buf + _buf_len, read_remainder);
    } else {
        terminal->read_buf_len = 0;
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
    //
    //    TODO: Do I need to call SchedulerUpdateTTYRead here?
    terminal_t *terminal = _tty->terminals[_tty_id];
    if (terminal->read_buf_len) {
        terminal->receive_count++;
        return 0;
    }

    // 3. If our terminal's read buffer is empty, then go ahead and read from the terminal and
    //    store its output in our read buffer. Update its length to the number of bytes read.
    int read_len = TtyReceive(_tty_id,
                              terminal->read_buf,
                              TERMINAL_MAX_LINE);
    if (read_len <= 0) {
        TracePrintf(1, "[TTYUpdateReadBuffer] Error TtyReceive returned bytes: %d\n", read_len);
        Halt();
    }
    terminal->read_buf_len = read_len;

    // 4. Check to see if we have a process waiting to read from the specified terminal.
    //    If so, remove them from the TTYRead wait list and add them to the ready list.
    SchedulerUpdateTTYRead(e_scheduler, _tty_id);
    return 0;
}

