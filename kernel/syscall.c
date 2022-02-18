#include <hardware.h>
#include <yalnix.h>
#include <ykernel.h>

#include "frame.h"
#include "kernel.h"
#include "process.h"
#include "scheduler.h"
#include "pte.h"
#include "syscall.h"


int SyscallFork (void) {
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
    return 0;
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
int SyscallExec (char *filename, char **argvec) {
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
    return 0;
}

void SyscallExit (int status) {
    // Free all the resources in the process's pcb
    // Check to see if the current process has a parent (parent pointer is null?), if so, save the `status` parameter into the pcb
    // Otherwise, free this pcb in the kernel (?) or put it on an orphan list (?)
    // Check if the process is the initial process, if so, halt the system.
}

int SyscallWait (int *status_ptr) {
    // Check to see if the process's children list is empty (children is a null pointer),
    // if so, return ERROR
    // Otherwise, walk through the process's children list and see if there's a process with `exited = 1`
    // If so, continue to next step; If not, block the current process until if its has a child with `exited=1`
    // Check if status_ptr is not null, collect its status and Save its exit status into `status_ptr`, otherwise just continue
    // retire its pid, delete it from the process's children list, then return
    // then, the call returns with the pid of that child.
    return 0;
}

int SyscallGetPid (void) {
    // 1. Get the current running process from our process list. If there is none,
    //    print a message and Halt. Otherwise, return the running process' pid.
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    if (!running) {
        TracePrintf(1, "[SyscallGetPid] e_scheduler returned no running process\n");
        Halt();
    }
    return running->pid;
}

int SyscallBrk (void *_brk) {
    // 1. Make sure the incoming address is not NULL. If so, return ERROR.
    if (!_brk) {
        TracePrintf(1, "[SyscallBrk] Error: proposed brk is NULL\n");
        return ERROR;
    }

    // 2. Get the PCB for the current running process. If our process list thinks that there is
    //    no current running process, print a message and halt. Otherwise, make sure that the
    //    proposed _brk is not less than our lower heap boundary (i.e., data end).
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    if (!running) {
        TracePrintf(1, "[SyscallBrk] e_scheduler returned no running process\n");
        Halt();
    }
    TracePrintf(1, "[SyscallBrk] running->brk: %p\t_brk: %p\n", running->brk, _brk);

    if (_brk <= running->data_end) {
        TracePrintf(1, "[SyscallBrk] Error: proposed brk is below heap base\n");
        return ERROR;
    }

    // 3. Round our new brk value *up* to the nearest page.
    // 
    //    NOTE: Currently, we do not maintain the exact brk value provided by the caller as it
    //          rounds the brk up to the nearest page boundary. In other words, the only valid
    //          brk values are those that fall on page boundaries, so the process may end up with
    //          *more* memory than it asked for. It should never end up freeing more memory than
    //          it specified, however, because we round up.
    //
    //    TODO: Do I need to round up? I need to think about this more...
    _brk = (void *) UP_TO_PAGE(_brk);

    // 4. If virtual memory has been enabled, then we are responsible for updating the process'
    //    page table to reflect any pages/frames that have been added/removed as a result of
    //    the brk change. Start off by calculating the page numbers for our new proposed brk
    //    and our current brk.
    int new_brk_page_num = ((int) _brk)         >> PAGESHIFT;
    int cur_brk_page_num = ((int) running->brk) >> PAGESHIFT;
    new_brk_page_num -= MAX_PT_LEN;
    cur_brk_page_num -= MAX_PT_LEN;

    // 5. Check to see if we are growing or shrinking the brk and calculate the number
    //    of pages that we either need to add or remove given the new proposed brk.
    int growing   = 0;
    int num_pages = 0;
    if (_brk > running->brk) {
        growing   = 1;
        num_pages = new_brk_page_num - cur_brk_page_num;
    } else {
        num_pages = cur_brk_page_num - new_brk_page_num;
    }

    // 6. Add or remove frames/pages based on whether we are growing or shrinking the heap.
    for (int i = 0; i < num_pages; i++) {

        if (growing) {
            // 6a. If we are growing the heap, then we first need to find an available frame.
            //     If we can't find one (i.e., we are out of memory) return ERROR.
            int frame_num = FrameFind();
            if (frame_num == ERROR) {
                TracePrintf(1, "[SyscallBrk] Unable to find free frame\n");
                return ERROR;
            }

            // 6b. Map the frame to a page in the process' page table. Specifically, start with the
            //     current page pointed to by the process brk. Afterwards, mark the frame as in use.
            PTESet(running->pt,             // page table pointer
                   cur_brk_page_num + i,    // page number
                   PROT_READ | PROT_WRITE,  // page protection bits
                   frame_num);              // frame number
            TracePrintf(1, "[SyscallBrk] Mapping page: %d to frame: %d\n",
                           cur_brk_page_num + i, frame_num);
        } else {
            // 6c. If we are shrinking the heap, then we need to unmap pages. Start by grabbing
            //     the number of the frame mapped to the current brk page. Free the frame.
            int frame_num = running->pt[cur_brk_page_num - i].pfn;
            FrameClear(frame_num);

            // 6d. Clear the page in the process' page table so it is no longer valid
            PTEClear(running->pt, cur_brk_page_num - i);
            TracePrintf(1, "[SyscallBrk] Unmapping page: %d from frame: %d\n",
                           cur_brk_page_num - i, frame_num);
        }
    }

    // 7. Set the process brk to the new brk value and return success
    TracePrintf(1, "[SyscallBrk] new_brk_page_num:  %d\n", new_brk_page_num);
    TracePrintf(1, "[SyscallBrk] cur_brk_page_num:  %d\n", cur_brk_page_num);
    TracePrintf(1, "[SyscallBrk] _brk:              %p\n", _brk);
    TracePrintf(1, "[SyscallBrk] running->brk:      %p\n", running->brk);
    running->brk = _brk;
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    return 0;
}


/*!
 * \desc                    A
 *
 * \param[in] _clock_ticks  a
 *
 * \return                  0 on success, ERROR otherwise.
 */
int SyscallDelay (UserContext *_uctxt, int _clock_ticks) {
    // 1. If delay is 0 return immediately and if delay is invalid return ERROR.
    if (!_uctxt || _clock_ticks < 0) {
        TracePrintf(1, "[SyscallDelay] Invalid uctxt pointer or clock_ticks value: %d\n", _clock_ticks);
        return ERROR;
    }
    if (_clock_ticks == 0) {
        return 0;
    }

    // 2. Get the pcb for the current running process and save its user context.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[SyscallDelay] e_scheduler returned no running process\n");
        Halt();
    }
    memcpy(running_old->uctxt, _uctxt, sizeof(UserContext));

    // 3. Set the current process' delay value in its pcb then add it to the blocked list
    running_old->clock_ticks = _clock_ticks;
    SchedulerAddDelay(e_scheduler, running_old);
    SchedulerPrintDelay(e_scheduler);
    TracePrintf(1, "[SyscallDelay] Blocking process %d for %d clock cycles\n",
                   running_old->pid, _clock_ticks);

    // 4. Get the next process from our ready queue and mark it as the current running process.
    //
    //    NOTE: We should *always* get a process back from the ready queue because at the
    //          very least it will contain the idleProc. We know this because idleProc
    //          never calls "delay" (or any other syscalls), thus if we are in delay we
    //          must be a different process and idleProc must be on the ready queue.
    pcb_t *running_new = SchedulerGetReady(e_scheduler);
    if (!running_new) {
        TracePrintf(1, "[SyscallDelay] e_scheduler returned no ready process\n");
        Halt();
    }
    SchedulerAddRunning(e_scheduler, running_new);

    // 5. Switch to the new process. If the new process has never been run before, KCSwitch will
    //    first call KCCopy to initialize the KernelContext for the new process and clone the
    //    kernel stack contents of the old process.
    TracePrintf(1, "[SyscallDelay] running_old->pid: %d\t\trunning_new->pid: %d\n",
                    running_old->pid, running_new->pid);
    int ret = KernelContextSwitch(KCSwitch,
                         (void *) running_old,
                         (void *) running_new);
    if (ret < 0) {
        TracePrintf(1, "[SyscallDelay] Failed to switch to the next process\n");
        Halt();
    }

    // 6. At this point, this code is being run by the *new* process, which means that its
    //    running_new stack variable is "stale" (i.e., running_new contains the pcb for the
    //    process that this new process previously gave up the CPU for). Thus, get the
    //    current running process (i.e., "this" process) and set the outgoing _uctxt.
    running_new = SchedulerGetRunning(e_scheduler);
    memcpy(_uctxt, running_new->uctxt, sizeof(UserContext));
    return 0;
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
int SyscallTtyRead (int tty_id, void *buf, int len) {
    // FEEDBACK: yes, kernel malloc would work! - for waiting... block the caller and run someone
    //           else.... and have the receive trap handler wake up the blocker! and similar
    //           comments for SyscallTtyWrite

    // // 1. Check arguments. Return ERROR if invalid. Make sure that (1) the terminal
    // //    id is valid (2) that buffer is not NULL and (3) length is positive.
    // if (tty_id < 0 || tty_id > 3 || !buf || len < 1) {
    //     return ERROR;
    // }

    // // 2. Page 25 states that buf should reside in kernel memory (specifically, virtual
    // //    memory region 0). I assume that means I should find space and copy the contents
    // //    of buf from the process' memory to the kernel memory here. TODO: How do I do that?
    // g_tty_read_buf[tty_id] = (void *) Syscallmalloc(len);

    // // 3. Write to the terminal specified by tty_id using the special hardware "operation"
    // //    TtyTransmit. TODO: The guide states that this function returns immediately but
    // //    will generate a trap when the write completes. What do I do here to "wait" for
    // //    the trap? Do I need some sort of global variable that I "spin" on that gets
    // //    reset in the trap handler?
    // void *kbuf = g_tty_read_buf[tty_id];
    // while (1) {

    //     // wait for the read this way?
    //     while (!g_tty_read_ready[tty_id]);

    //     if (len > TERMINAL_MAX_LINE) {
    //         TtyReceive(tty_id,
    //                    kbuf,
    //                    TERMINAL_MAX_LINE);
    //         kbuf += TERMINAL_MAX_LINE;
    //         len  -= TERMINAL_MAX_LINE;
    //     } else {
    //         TtyReceive(tty_id,
    //                    kbuf,
    //                    len);
    //         break;
    //     }
    // }

    // // Reset this so other processes can use terminal? Also, do I need variable to indicate
    // // this terminal is in use and other processes should not use it?
    // g_tty_read_ready[tty_id] = 1;
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
int SyscallTtyWrite (int tty_id, void *buf, int len) {
    // FEEDBACK: yes, kernel malloc would work! - for waiting... block the caller and run someone
    //           else.... and have the receive trap handler wake up the blocker! and similar
    //           comments for SyscallTtyWrite
    // // 1. Check arguments. Return ERROR if invalid. Make sure that (1) the terminal
    // //    id is valid (2) that buffer is not NULL and (3) length is positive.
    // if (tty_id < 0 || tty_id > 3 || !buf || len < 1) {
    //     return ERROR;
    // }

    // // 2. Page 25 states that buf should reside in kernel memory (specifically, virtual
    // //    memory region 0). I assume that means I should find space and copy the contents
    // //    of buf from the process' memory to the kernel memory here. TODO: How do I do that?
    // g_tty_write_buf[tty_id] = (void *) Syscallmalloc(len);

    // // 3. Write to the terminal specified by tty_id using the special hardware "operation"
    // //    TtyTransmit. TODO: The guide states that this function returns immediately but
    // //    will generate a trap when the write completes. What do I do here to "wait" for
    // //    the trap? Do I need some sort of global variable that I "spin" on that gets
    // //    reset in the trap handler?
    // void *kbuf = g_tty_write_buf[tty_id];
    // while (1) {

    //     // wait for the write this way?
    //     while (!g_tty_write_ready[tty_id]);

    //     if (len > TERMINAL_MAX_LINE) {
    //         TtyTransmit(tty_id,
    //                     kbuf,
    //                     TERMINAL_MAX_LINE);
    //         kbuf += TERMINAL_MAX_LINE;
    //         len  -= TERMINAL_MAX_LINE;
    //     } else {
    //         TtyTransmit(tty_id,
    //                     kbuf,
    //                     len);
    //         break;
    //     }
    // }

    // // Reset this so other processes can use terminal? Also, do I need variable to indicate
    // // this terminal is in use and other processes should not use it?
    // g_tty_write_ready[tty_id] = 1;
    return 0;
}


/*!
 * \desc                 Creates a new pipe.
 *
 * \param[out] pipe_idp  The address where the newly created pipe's id should be stored
 *
 * \return               0 on success, ERROR otherwise
 */
int SyscallPipeInit (int *pipe_idp) {
    // // 1. Check arguments. Return ERROR if invalid.
    // if (!pipe_idp) {
    //     return ERROR;
    // }

    // // 2. Initialize a new pipe structure
    // pipe_t *newpipe = (pipe_t *) Syscallmalloc(sizeof(pipe_t));
    // newpipe->id     = g_pipes_len++;
    // newpipe->plen   = 0;
    // newpipe->read   = 0;
    // newpipe->write  = 0;
    // newpipe->buf    = (void *) Syscallmalloc(PIPE_BUFFER_LEN); 

    // // 3. Add new pipe to our global pipe array/table/structure to keep track

    // // 4. Save the pipe id to the output variable
    // *pipe_idp = newpipe->id;
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
int SyscallPipeRead (int pipe_id, void *buf, int len) {
    // // 1. Check arguments. Return ERROR if invalid. Make sure that (1) the pipe id is
    // //    valid (2) that the buffer is not NULL and (3) that len is not negative.
    // if (pipe_id < 0 || pipe_id > g_pipes_len || !buf || len < 0) {
    //     return ERROR;
    // }

    // // 2. Check to see if pipe is empty. If so, block. TODO: How?
    // pipe_t *pipe = g_pipes[pipe_id];
    // if (pipe->plen == 0) {
    //     // block the calling process
    // }

    // // 3. If plen <= len, give the caller all the bytes in the pipe.
    // //    Otherwise, return them len bytes.
    // int read = len;
    // if (pipe->plen <= len) {
    //     read = pipe->plen;
    // }

    // // 4. Treat pipe internal buffer as a circular queue. Use the start variable and
    // //    pipe length to calculate correct index. Copy bytes to output array. TODO:
    // //    I dont know if this calculation is correct now that I realize we have to
    // //    track where to read and where to write.
    // for (int i = 0; i < read; i++) {
    //     int index = (pipe->read + i) % PIPE_BUFFER_LEN;
    //     buf[i] = pipe->[index];
    // }

    // // 5. Update pipe queue start and length variables
    // pipe->read  = (pipe->read + read) % PIPE_BUFFER_LEN;
    // pipe->plen -= read;
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
int SyscallPipeWrite (int pipe_id, void *buf, int len) {
    // // 1. Check arguments. Return ERROR if invalid. Make sure that (1) the pipe id is
    // //    valid (2) that the buffer is not NULL and (3) that len is not negative.
    // if (pipe_id < 0 || pipe_id > g_pipes_len || !buf || len < 0) {
    //     return ERROR;
    // }

    // FEEDBACK: No need to block on an empty pipe! What if there is a blocked reader tho?
    // // 2. Check to see if pipe is empty. If so, block. TODO: How?
    // pipe_t *pipe = g_pipes[pipe_id];
    // if (pipe->plen == 0) {
    //     // block the calling process
    // }

    // // 3. If plen <= len, give the caller all the bytes in the pipe.
    // //    Otherwise, return them len bytes.
    // int write = len;
    // if (pipe->plen <= len) {
    //     write = pipe->plen;
    // }

    // // 4. Treat pipe internal buffer as a circular queue. Use the end variable and
    // //    pipe length to calculate correct index. Copy bytes to pipe buffer. TODO:
    // //    I dont know if this calculation is correct now that I realize we have to
    // //    track where to read and where to write.
    // for (int i = 0; i < write; i++) {
    //     int index = (pipe->write + i) % PIPE_BUFFER_LEN;
    //     pipe->[index] = buf[i];
    // }

    // // 5. Update pipe queue end and length variables.
    // pipe->write  = (pipe->write + write) % PIPE_BUFFER_LEN;
    // pipe->plen  += write;
    return 0;
}


/*!
 * \desc                 Creates a new lock.
 *
 * \param[out] lock_idp  The address where the newly created lock's id should be stored
 *
 * \return               0 on success, ERROR otherwise
 */
int SyscallLockInit (int *lock_idp) {
    // // 1. Check arguments. Return ERROR if invalid.
    // if (!lock_idp) {
    //     return ERROR;
    // }

    // // 2. Initialize a new lock structure
    // lock_t *newlock  = (lock_t *) Syscallmalloc(sizeof(lock_t));
    // newlock->id      = g_locks_len++;
    // newlock->owner   = 0;
    // newlock->waiting = NULL;

    // // 3. Add new lock to our global locks array/table/structure to keep track

    // // 4. Save the lock id to the output variable
    // *lock_idp = newlock->id;
    return 0;
}


/*!
 * \desc               Acquires the lock identified by lock_id
 *
 * \param[in] lock_id  The id of the lock to be acquired
 *
 * \return             0 on success, ERROR otherwise
 */
int SyscallAcquire (int lock_id) {
    // // 1. Check arguments. Return ERROR if invalid. The lock id should
    // //    be within 0 and the total number of initialized locks.
    // if (lock_id < 0 || lock_id > g_locks_len) {
    //     return ERROR;
    // }

    // // 2. Check to see if the lock is already owned. If so, add the
    // //    current process to the waiting queue and return ERROR.
    // lock_t *lock = g_locks[lock_id];
    // if (lock->owner) {
    //     lock->waiting = g_current->pid;
    //     return ERROR;
    // }

    // // 3. Mark the current process as the owner of the lock
    // lock->owner = g_current->pid;
    return 0;
}


/*!
 * \desc               Releases the lock identified by lock_id
 *
 * \param[in] lock_id  The id of the lock to be released
 *
 * \return             0 on success, ERROR otherwise
 */
int SyscallRelease (int lock_id) {
    // FEEDBACK: What if someone was blocked waiting for this lock?
    // // 1. Check arguments. Return ERROR if invalid. The lock id should
    // //    be within 0 and the total number of initialized locks.
    // if (lock_id < 0 || lock_id > g_locks_len) {
    //     return ERROR;
    // }

    // // 2. Check to see that the current process actually owns the lock.
    // //    If not, do nothing and return ERROR.
    // lock_t *lock = g_locks[lock_id];
    // if (lock->owner != g_current->pid) {
    //     return ERROR;
    // }

    // // 3. Mark the lock as free (use 0 to indicate free and PID as taken)
    // lock->owner = 0;
    return 0;
}


/*!
 * \desc                 Creates a new condition variable.
 *
 * \param[out] cvar_idp  The address where the newly created cvar's id should be stored
 *
 * \return               0 on success, ERROR otherwise
 */
int SyscallCvarInit (int *cvar_idp) {
    // // 1. Check arguments. Return ERROR if invalid.
    // if (!cvar_idp) {
    //     return ERROR;
    // }

    // // 2. Initialize a new lock structure
    // cvar_t *newcvar  = (cvar_t *) Syscallmalloc(sizeof(cvar_t));
    // newcvar->id      = g_cvars_len++;
    // newlock->waiting = NULL;

    // // 3. Add new cvar to our global cvar array/table/structure to keep track

    // // 4. Save the lock id to the output variable
    // *cvar_idp = newcvar->id;
    return 0;
}


/*!
 * \desc               Signal the condition variable identified by cvar_id
 *
 * \param[in] cvar_id  The id of the cvar to be signaled
 *
 * \return             0 on success, ERROR otherwise
 */
int SyscallCvarSignal (int cvar_id) {
    // // 1. Check arguments. Return ERROR if invalid. The cvar id should
    // //    be within 0 and the total number of initialized cvars.
    // if (cvar_id < 0 || cvar_id > g_cvars_len) {
    //     return ERROR;
    // }
    return 0;

    // 2. Signal the first waiting process for the cvar indicated by cvar_id.
    //    What exactly does that look like?
    // g_cvars->waiting[0]

    // 3. Remove the signaled process from the waiting list
}

/*!
 * \desc               Broadcast the condition variable identified by cvar id.
 *
 * \param[in] cvar_id  The id of the cvar to be broadcast
 *
 * \return             0 on success, ERROR otherwise
 */
int SyscallCvarBroadcast (int cvar_id) {
    // Check if the cvar_id exists? Return ERROR if not
    // for each process in the cvar_id's waiting queue, remove it from the waiting queue and put it on the ready_queue
    // Mesa style: Caller continues to execute
    // Return 0
    return 0;
}

/*!
 * \desc               The kernel-level process releases the lock identified by lock id and waits on the condition variable indentified by cvar id
 *
 * \param[in] cvar_id  The id of the cvar to be broadcast
 *
 * \return             0 on success, ERROR otherwise
 */
int SyscallCvarWait (int cvar_id, int lock_id) {
    // Assert the lock of `lock_id` is held by the current process
    // Release the lock identified by lock_id
    // Add the current process to the waiting queue of the cvar_id
    // Ask the scheduler to choose a process from the ready queue and KernelContextSwitch to it
    // The above steps probably can be wrapped in a function `Suspend`, See Textbook Page 243
    // Acquire the lock of `lock_id`
    return 0;
}

/*!
 * \desc               Destroy the lock, condition variable, or pipe indentified by id, and release any associated resources. 
 *
 * \param[in] cvar_id  The id of the lock, cvar or pipe
 *
 * \return             0 on success, ERROR otherwise
 */

int SyscallReclaim (int id) {
    // Check which type of primitive the `id` is of
    // Free its malloc'ed memory
    return 0;
}
