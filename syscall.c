#include "syscall.h"

int internal_Fork (void) {
    // Create a new pcb for child process
    // Get a new pid for the child process
    // Copy user_context into the new pcb
    // Call KernelContextSwitch(KCCopy, *new_pcb, NULL) to copy the current process into the new pcb
    // 

}


/*!
 * \desc                Replaces the currently running program in the calling process' memory with
 *                      the program stored in the file named by filename. The argumetn argvec
 *                      points to a vector of arguments to pass to the new program as its argument
 *                      list.
 * 
 * \param[in] filename  The file containing the new program to instantiate
 * \param[in] argvec    The address of the vector containing arguments to the new program
 * 
 * \return              0 on success, ERROR otherwise
 */
int internal_Exec (char *filename, char **argvec) {
    // 1. Check arguments. Return error if invalid.
    if (!filename || !argvec) {
        return ERROR;
    }
    return 0;
}

void internal_Exit (int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Wait (int internal_*) {
    // 1. Check arguments. Return error if invalid.
}

int internal_GetPid (void) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Brk (void *) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Delay (int) {
    // 1. Check arguments. Return error if invalid.
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
int internal_TtyRead (int tty_id, void *buf, int len) {
    // 1. Check arguments. Return error if invalid.
    if (!buf) {
        return ERROR;
    }
    return 0;
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
int internal_TtyWrite (int tty_id, void *buf, int len) {
    // 1. Check arguments. Return error if invalid.
    if (!buf) {
        return ERROR;
    }
    return 0;
}


/*!
 * \desc                 Creates a new pipe.
 * 
 * \param[out] pipe_idp  The address where the newly created pipe's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int internal_PipeInit (int *pipe_idp) {
    // 1. Check arguments. Return error if invalid.
    if (!pipe_idp) {
        return ERROR;
    }
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
int internal_PipeRead (int pipe_id, void *buf, int len) {
    // 1. Check arguments. Return error if invalid.
    if (!buf) {
        return ERROR;
    }
    return 0;
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
int internal_PipeWrite (int pipe_id, void *buf, int len) {
    // 1. Check arguments. Return error if invalid.
    if (!buf) {
        return ERROR;
    }
    return 0;
}


/*!
 * \desc                 Creates a new lock.
 * 
 * \param[out] lock_idp  The address where the newly created lock's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int internal_LockInit (int *lock_idp) {
    // 1. Check arguments. Return error if invalid.
    if (!lock_idp) {
        return ERROR;
    }
    return 0;
}


/*!
 * \desc               Acquires the lock identified by lock_id
 * 
 * \param[in] lock_id  The id of the lock to be acquired
 * 
 * \return             0 on success, ERROR otherwise
 */
int internal_Acquire (int lock_id) {
    // 1. Check arguments. Return error if invalid.
    return 0;
}


/*!
 * \desc               Releases the lock identified by lock_id
 * 
 * \param[in] lock_id  The id of the lock to be released
 * 
 * \return             0 on success, ERROR otherwise
 */
int internal_Release (int lock_id) {
    // 1. Check arguments. Return error if invalid.
    return 0;
}


/*!
 * \desc                 Creates a new condition variable.
 * 
 * \param[out] cvar_idp  The address where the newly created cvar's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int internal_CvarInit (int *cvar_idp) {
    // 1. Check arguments. Return error if invalid.
    if (!cvar_idp) {
        return ERROR;
    }
    return 0;
}


/*!
 * \desc               Signal the condition variable identified by cvar_id
 * 
 * \param[in] cvar_id  The id of the cvar to be signaled
 * 
 * \return             0 on success, ERROR otherwise
 */
int internal_CvarSignal (int cvar_id) {
    // 1. Check arguments. Return error if invalid.
    return 0;
}

int internal_CvarBroadcast (int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_CvarWait (int, int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Reclaim (int) {
    // 1. Check arguments. Return error if invalid.
}