#include <hardware.h>
#include <yalnix.h>
#include <ykernel.h>

#include "frame.h"
#include "load_program.h"
#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "syscall.h"
#include "tty.h"


/*!
 * \desc                Fork a new process based on the caller process's current setup
 *
 * \param[in] _uctxt    current UserContext
 *
 * \return              Return child process's pid for parent, 0 for child process, and ERROR otherwise
 */
int SyscallFork (UserContext *_uctxt) {
    // Create a new pcb for child process, and get a new pid for the child process
    pcb_t *child = ProcessCreate();
    if (child == NULL) {
        TracePrintf(1, "SyscallFork: failed to create a new process.\n");
        return ERROR;
    }
    SchedulerAddProcess(e_scheduler, child);
    // Copy user_context into the new pcb
    memcpy(&child->uctxt, _uctxt, sizeof(UserContext));
    // For each valid pte in the page table of the parent process, find a free frame and change the page table entry to map into this frame
    pcb_t *parent = SchedulerGetRunning(e_scheduler);
    for (int i = 0; i < MAX_PT_LEN; i++) {
        if (parent->pt[i].valid) {
            int pfn = FrameFindAndSet();
            if (pfn == ERROR) {
                TracePrintf(1, "SyscallFork: failed to find a free frame.\n");
                ProcessDestroy(child);
                return ERROR;
            }
            PTESet(child->pt, i, (int) parent->pt[i].prot, pfn);
            // Copy the frame stack to a new frame stack by temporarily mapping the destination frame into some page
            // E.g, the page right below the kernel stack.
            void *src_addr = (void *) ((i << PAGESHIFT) + VMEM_1_BASE);
            int temp_page_num = (KERNEL_STACK_BASE >> PAGESHIFT) - 1;
            // Assert kernel heap is at least one page below kernel stack
            void *temp_page_addr = (void *) (temp_page_num << PAGESHIFT);
            if (temp_page_addr < e_kernel_curr_brk) {
                TracePrintf(1, "SyscallFork: unable to use the frame below kernel stack as a temporary.\n");
                ProcessDestroy(child);
                return ERROR;
            }
            // Copy the frame from parent to child
            PTESet(e_kernel_pt, temp_page_num, PROT_READ|PROT_WRITE, pfn);
            memcpy(temp_page_addr, src_addr, PAGESIZE);
            PTEClear(e_kernel_pt, temp_page_num);
            WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
        }
    }
    // Setup relationship between parent/child
    ProcessAddChild(parent, child);
    child->parent = parent;
    // Add child to ready list
    SchedulerAddReady(e_scheduler, child);

    // Call KernelContextSwitch(KCCopy, *new_pcb, NULL) to copy the current process into the new pcb
    if (child->kctxt != NULL) { TracePrintf(1, "SyscallFork: child->kctxt should be null\n"); Halt();}
    if (KernelContextSwitch(KCCopy, child, NULL) == ERROR) {
        TracePrintf(1, "SyscallFork: KernelContextSwitch failed.\n");
        Halt();
    }
    // Make the return value different between parent process and child process
    pcb_t *cur_running = SchedulerGetRunning(e_scheduler);
    if (cur_running == parent) {
        return child->pid;
    } else {
        return 0;
    }
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
int SyscallExec (UserContext *_uctxt, char *_filename, char **_argvec) {
    // 1. Check filename argument for NULL
    if (!_filename || !_argvec || !_argvec[0]) {
        TracePrintf(1, "[SyscallExec] One or more invalid arguments\n");
        return ERROR;
    }

    // 2. Get the current running process from our process list. If there is
    //    none, print a message and Halt because something has gone wrong. 
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    if (!running) {
        TracePrintf(1, "[SyscallExec] e_scheduler returned no running process\n");
        Halt();
    }

    // 3. Calculate the length of the filename string and then check that it is within valid
    //    region 1 memory space for the current running process. Specifically, check that every
    //    byte is within region 1 and that they are in pages that have valid protections.
    int length = 0;
    while (_filename[length] != '\0') { length++; }
    int ret = PTECheckAddress(running->pt,
                     (void *) _filename,
                              length,
                              PROT_READ);
    if (ret < 0) {
        TracePrintf(1, "[SyscallExec] Filename is not within valid address space\n");
        PTEPrint(running->pt);
        Halt();
    }

    // 4. Calculate the number of arguments. For each argument, calculate its length then check
    //    that it is within valid region 1 memory space.
    int num_args = 0;
    if (_argvec) {
        while (_argvec[num_args]) { num_args++; }
    }
    for (int i = 0; i < num_args; i++) {
        length = 0;
        while (_argvec[i][length] != '\0') { length++; }
        ret = PTECheckAddress(running->pt,
                     (void *) _argvec[i],
                              length,
                              PROT_READ);
        if (ret < 0) {
            TracePrintf(1, "[SyscallExec] Argvec[%d] is not within valid address space\n", i);
            PTEPrint(running->pt);
            Halt();
        }
    }

    // 5. Call LoadProgram which will copy over _filename and the argvec to kernel memory, free
    //    all current pages in the page table, then load the program into memory and reallocate
    //    pages for the new program. It will also set its UserContext for the new pc and sp.
    //
    //    If LoadProgram is successfull, copy the update UserContext into the yalnix system's
    //    UserContext variable so that when we return we begin executing at the newly loaded
    //    program's code instead of the code that we just replaced.
    ret = LoadProgram(_filename, _argvec, running);
    if (ret < 0) {
        TracePrintf(1, "[SyscallExec] Error loading program: %s\n", _filename);
        Halt();
    }
    memcpy(_uctxt, &running->uctxt, sizeof(UserContext));
    return 0;
}

void SyscallExit (UserContext *_uctxt, int _status) {
    // 1. Get the current running process from our process list. If there is
    //    none, print a message and Halt because something has gone wrong. 
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    if (!running) {
        TracePrintf(1, "[SyscallExit] e_scheduler returned no running process\n");
        Halt();
    }

    // 2. If the initial process exits then we should halt the system. Since our idle and init
    //    processes are setup in KernelStart, we know that their pids are 0 and 1 respectively.
    //    Thus, check if the running process pid is equal to either of these. If so, halt.
    if (running->pid < 2) {
        TracePrintf(1, "[SyscallExit] Idle or Init process called Exit. Halting system\n");
        Halt();
    }

    // 3. If we have no runnning parent, then we do not need to save our exit status or keep
    //    track of our pcb for future use. Instead, free all frames and pcb process memory.
    if (!running->parent) {

        // Before we free all of our resources, check to see if we have any children. If so,
        // update the terminated list. More specifically, check to see if any of our children
        // have exited and their states are stored in the terminated list---if they are, remove
        // their pcbs from the terminated list and free them. If we did not do this then our
        // Terminated list could potentially grow forever and cause the system to crash.
        TracePrintf(1, "[SyscallExit] Terminated list before update\n");
        SchedulerPrintTerminated(e_scheduler);
        if (running->headchild) {
            TracePrintf(1, "[SyscallExit] Terminated list before update\n");
            SchedulerPrintTerminated(e_scheduler);
            SchedulerUpdateTerminated(e_scheduler, running);
            TracePrintf(1, "[SyscallExit] Terminated list after update\n");
            SchedulerPrintTerminated(e_scheduler);
        }

        // Remove ourselves from the master process list and free our pcb memory. If we have any
        // living children, ProcessDestroy will update their parent pointers to NULL so that they
        // do not add themselves to the Terminated list when they exit.
        TracePrintf(1, "[SyscallExit] Process %d deleted with status %d\n", running->pid, _status);
        SchedulerRemoveProcess(e_scheduler, running->pid);
        ProcessDestroy(running);
        KCSwitch(_uctxt, NULL);
    }

    // 4. Otherwise, we want to save the process' exit status and "terminate" it. The difference
    //    between termination and deletion is that termination frees the process' region 1 and
    //    kernel stack frames (but not the pcb), whereas deletion frees frames and pcb memory.
    //    The reason we want to save the pcb is that it holds information needed by the parent.
    //
    //    So "terminate" the process, add it to our terminated list, then update the wait list---
    //    update will check to see if the children's parent process is waiting on them, and if so,
    //    move the parent over to the ready list. Later on, the parent resume in SyscallWait and
    //    find its child's pcb in the terminated list.
    TracePrintf(1, "[SyscallExit] Process %d terminated with status %d\n", running->pid, _status);
    running->exited      = 1;
    running->exit_status = _status;
    ProcessTerminate(running);
    SchedulerAddTerminated(e_scheduler, running);
    SchedulerUpdateWait(e_scheduler, running->parent->pid);
    SchedulerPrintTerminated(e_scheduler);
    SchedulerPrintWait(e_scheduler);
    SchedulerPrintReady(e_scheduler);

    // 5. The guide states that this call should never return. Thus, we should context switch
    //    to the next ready process. Since the exited process is now in the terminated list,
    //    it will never again be added to ready and thus will never run (i.e., return).
    KCSwitch(_uctxt, running);
}

int SyscallWait (UserContext *_uctxt, int *_status_ptr) {
    // 1. Get the current running process
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    if (!running) {
        TracePrintf(1, "[SyscallWait] e_scheduler returned no running process\n");
        Halt();
    }

    // 2. If the parent has no remaining child processes, return immediately.
    if (!running->headchild) {
        TracePrintf(1, "[SyscallWait] Parent %d has no remaining children\n", running->pid);
        return ERROR;
    }

    // 3. Check that the address of the status pointer is within valid memory space. Note that we
    //    want to ensure every *byte* of _status is within valid memory space. Thus, we convert it
    //    to a void buffer (void is 1 byte each) that is the length of an int. This way, a user
    //    cannot pass us an address of the last byte of a valid page in hopes of writing to the
    //    next (potentially invalid) page because an integer is larger than a byte.
    int ret = PTECheckAddress(running->pt,
                     (void *) _status_ptr,
                              sizeof(int),
                              PROT_READ | PROT_WRITE);
    if (ret < 0) {
        TracePrintf(1, "[SyscallWait] Status pointer is not within valid address space\n");
        return ERROR;
    }

    // If our code works correctly, then this outer while loop should never actually run more than
    // two times. Specifically, a process might loop through all of its children but see that none
    // are finished. So, it blocks itself and cedes the CPU to the next ready process. At some
    // point in time one of its children finishes and *sees* that its parent is in the wait queue.
    // before exiting, the child moves the parent to the ready queue. When the parent runs again,
    // it loops back once more but this time should find a child in the terminated queue and return
    // from this function.
    while (1) {

        // 3. Loop through the current running process' children to see if any have finished.
        pcb_t *child = running->headchild;
        while (child) {
            int child_pid = child->pid;

            // If we find a child in the terminated list, first remove it so that we do
            // not "find" it again in successive calls to "Wait". Save its exit status
            // in the outgoing status_ptr, free its pcb memory, and return its pid.
            if (SchedulerGetTerminated(e_scheduler, child_pid)) {
                TracePrintf(1, "[SyscallWait] Removing child %d from terminated list\n", child_pid);
                SchedulerPrintTerminated(e_scheduler);
                SchedulerRemoveTerminated(e_scheduler, child_pid);
                SchedulerPrintTerminated(e_scheduler);
                if (_status_ptr) {
                    *_status_ptr = child->exit_status;
                }
                ProcessDelete(child);
                return child_pid;
            }
            child = child->sibling;
        }

        // 4. If none of the running process' children have finished, then save its UserContext,
        //    add it to the wait queue and let the next ready process run. The parent process will
        //    get moved back onto the ready queue once one of its children exits (this may happen
        //    in SyscallExit, TrapMemory, or any function where a process is termianted).
        TracePrintf(1, "[SyscallWait] Parent %d waiting for a child to finish\n", running->pid);
        memcpy(&running->uctxt, _uctxt, sizeof(UserContext));
        SchedulerAddWait(e_scheduler, running);
        SchedulerPrintWait(e_scheduler);
        KCSwitch(_uctxt, running);
    }

    // 5. This should never be reached.
    return ERROR;
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

int SyscallBrk (UserContext *_uctxt, void *_brk) {
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
    _brk = (void *) UP_TO_PAGE(_brk);

    // 4. Calculate the page numbers for the process' region 1 stack, current brk, and new brk.
    //    The guide advises us to leave a "red" zone between the heap and stack to prevent them
    //    from overwriting each other. Check to see that the new brk leaves enough space between
    //    the stack. If not, print a message and return ERROR.
    int stack_page_num   = PTEAddressToPage(_uctxt->sp)   - MAX_PT_LEN;
    int cur_brk_page_num = PTEAddressToPage(running->brk) - MAX_PT_LEN;
    int new_brk_page_num = PTEAddressToPage(_brk)         - MAX_PT_LEN;
    if (new_brk_page_num >= stack_page_num - KERNEL_NUMBER_STACK_FRAMES) {
        TracePrintf(1, "[SyscallBrk] Error: proposed brk is in red zone.\n");
        return ERROR;
    }

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
            int frame_num = FrameFindAndSet();
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
    memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));

    // 3. Set the current process' delay value in its pcb then add it to the blocked list
    running_old->clock_ticks = _clock_ticks;
    SchedulerAddDelay(e_scheduler, running_old);
    TracePrintf(1, "[SyscallDelay] Blocking process %d for %d clock cycles\n",
                   running_old->pid, _clock_ticks);

    return KCSwitch(_uctxt, running_old);
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
