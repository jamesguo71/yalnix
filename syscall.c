#include "syscall.h"

int internal_Fork (void) {
    // Reference Checkpoint 3 for more details
    // Create a new pcb for child process
    // Get a new pid for the child process
    // Copy user_context into the new pcb
    // Call KernelContextSwitch(KCCopy, *new_pcb, NULL) to copy the current process into the new pcb
    // For each valid pte in the page table of the parent process, find a free frame and change the page table entry to map into this frame
    // Copy the frame from parent to child
    // Copy the frame stack to a new frame stack by temporarily mapping the destination frame into some page
    // E.g, the page right below the kernel stack.
    // Make the return value different between parent process and child process
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
    // Should be pretty similar to what LoadProgram in `template.c` does
    // Open the file
    // Calculate the start and end position of each section (Text, Data, Stack)
    // Reserve space for argc and argv on the stack
    // Throw away the old page table and build up the new one
    // Flush TLB
    // Read the data from file into memory
    // Change the PROT field of the TEXT section
    // Set the entry point in the process's UserContext
    // Build the argument list on the new stack.
}

void internal_Exit (int status) {
    // Free all the resources in the process's pcb
    // Check to see if the current process has a parent (parent pointer is null?), if so, save the `status` parameter into the pcb
    // Otherwise, free this pcb in the kernel (?) or put it on an orphan list (?)
    // Check if the process is the initial process, if so, halt the system.
}

int internal_Wait (int *status_ptr) {
    // Check to see if the process's children list is empty (children is a null pointer),
    // if so, return ERROR
    // Otherwise, walk through the process's children list and see if there's a process with `exited = 1`
    // If so, continue to next step; If not, block the current process until if its has a child with `exited=1`
    // Check if status_ptr is not null, collect its status and Save its exit status into `status_ptr`, otherwise just continue
    // retire its pid, delete it from the process's children list, then return
    // then, the call returns with the pid of that child.
}

int internal_GetPid (void) {
    // Return the pid of the current pcb
}

int internal_Brk (void *addr) {
    // Check if addr is greater or less than the current brk of the process
    // If greater, check if it is below the Userland Red Zone
        // Return ERROR if not
        // Call UP_TO_PAGE to get the suitable new addr
        // Adjust page table to account for the new brk
    // If less, check if it is above the USER DATA limit
        // Return ERROR if not
        // Call DOWN_TO_PAGE to get the suitable new addr
        // Adjust page table to account for the new brk
    return 0
}

int internal_Delay (int clock_ticks) {
    // If clock ticks is 0, return is immediate.
    // If clock ticks is less than 0, return ERROR
    // Block current process until clock_ticks clock interrupts have occurred after the call
    // Upon completion of the delay, the value 0 is returned
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