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
    int   read_pid;
    int   write_pid;
    pcb_t *write_proc;
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

    // 5. Check to see if there is data ready to read. If not, mark the current process as the
    //    next one that should get to read and block it until the read buf is populated.
    if (!terminal->read_buf_len) {
        if (terminal->read_pid) {
            TracePrintf(1, "[TTYRead] Error there is already a process on tty_id: %d\n", _tty_id);
            Halt();
        }
        TracePrintf(1, "[TTYRead] tty_id: %d read_buf empty. Blocking process: %d\n",
                                  _tty_id, running_old->pid);
        running_old->tty_id = _tty_id;
        terminal->read_pid  = running_old->pid;
        memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddTTYRead(e_scheduler, running_old);
        SchedulerPrintTTYRead(e_scheduler);
        KCSwitch(_uctxt, running_old);
    }

    // 6. At this point, the read_buf should be populated with data (populated by TrapTTYReceive).
    //    Write the tty data into the user's output buffer (first figure out how much to read).
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

    // TODO: So after reading the guide more I think this is wrong. Specifically, I think our
    //       internal read buffer needs to be variable length because it is possible that a
    //       terminal gets written to *many* times before anybody ever reads from it; I thought
    //       that TERMINAL_MAX_LINE was meant to be the length of our internal buffer, but this
    //       is actually the maximum characters that a single call to TtyReceive could return, and
    //       our internal buffer may need to store multiple lines. Thus, update the code here (and
    //       in TTYRead to)
    // 2. Read from the specified terminal and store its output in our read buffer
    terminal_t *terminal = _tty->terminals[_tty_id];
    int read_len = TtyReceive(_tty_id,
                              terminal->read_buf,
                              TERMINAL_MAX_LINE);
    if (read_len <= 0) {
        TracePrintf(1, "[TTYUpdateReadBuffer] TtyReceive returned negative bytes: %d\n", read_len);
        Halt();
    }
    terminal->read_buf_len = read_len;

    // 3.
    SchedulerUpdateTTYRead(e_scheduler, _tty_id);
    return 0;
}

