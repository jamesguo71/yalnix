#include "hardware.h"
#include "kernel.h"
#include "syscall.h"
#include "trap.h"
#include "yalnix.h"
#include "ylib.h"


/*!
 * \desc               Calls the appropriate internel syscall function based on the current process
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapKernel(UserContext *_uctxt) {
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }

    // 2. Page. 36 of the manual states that the "code" field in UserContext will contain
    //    the number of the syscal (as defined in yalnix.h). Additionally, any arguments
    //    to the syscall will be placed in the "regs" array beginning with reg[0]. Use
    //    this information to dispatch the appropriate syscall function. Afterwards, store
    //    the syscall return value in reg[0].
    u_long ret = 0;
    switch(_uctxt->code) {
        case YALNIX_FORK:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallFork();
            // _uctxt->regs[0] = ret;

        case YALNIX_EXEC:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallExec((char *)  _uctxt->regs[0],
            //                     (char **) _uctxt->regs[1]);
            // _uctxt->regs[0] = ret;

        case YALNIX_EXIT:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // SyscallExit((int ) _uctxt->regs[0]);

        case YALNIX_WAIT:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallWait((int *) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        case YALNIX_GETPID:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            ret = SyscallGetPid();
            _uctxt->regs[0] = ret;

        case YALNIX_BRK:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            ret = SyscallBrk((void *) _uctxt->regs[0]);
            _uctxt->regs[0] = ret;

        case YALNIX_DELAY:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallDelay((int ) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        case YALNIX_TTY_READ:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallTtyRead((int )   _uctxt->regs[0],
            //                        (void *) _uctxt->regs[1],
            //                        (int)    _uctxt->regs[2]);
            // _uctxt->regs[0] = ret;

        case YALNIX_TTY_WRITE:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallTtyWrite((int)    _uctxt->regs[0],
            //                         (void *) _uctxt->regs[1],
            //                         (int )   _uctxt->regs[2]);
            // _uctxt->regs[0] = ret;

        case YALNIX_PIPE_INIT:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallPipeInit((int *) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        case YALNIX_PIPE_READ:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallPipeRead((int)    _uctxt->regs[0],
            //                         (void *) _uctxt->regs[1],
            //                         (int)    _uctxt->regs[2]);
            // _uctxt->regs[0] = ret;

        case YALNIX_PIPE_WRITE:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallPipeWrite((int)    _uctxt->regs[0],
            //                          (void *) _uctxt->regs[1],
            //                          (int)    _uctxt->regs[2]);
            // _uctxt->regs[0] = ret;

        case YALNIX_LOCK_INIT:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallLockInit((int *) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        case YALNIX_LOCK_ACQUIRE:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallAcquire((int ) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        case YALNIX_LOCK_RELEASE:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallRelease((int) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        case YALNIX_CVAR_INIT:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallCvarInit((int *) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        case YALNIX_CVAR_SIGNAL:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallCvarSignal((int ) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        case YALNIX_CVAR_BROADCAST:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallCvarBroadcast((int ) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        case YALNIX_CVAR_WAIT:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallCvarWait((int ) _uctxt->regs[0],
            //                         _uctxt->regs[1]);
            // _uctxt->regs[0] = ret;

        case YALNIX_RECLAIM:
            TracePrintf(1, "[TrapKernel] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallReclaim((int ) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;

        default: break;
    }
    return 0;
}

/*!
 * \desc               If there are other runnable processes on the ready queue, perform a _uctxt
 *                     switch to the next runnable process. 
 *                     
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapClock(UserContext *_uctxt) {    
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        TracePrintf(1, "[TrapClock] Invalid UserContext pointer\n");
        return ERROR;
    }

    // 2. Get the pcb for the current running process and save its user context.
    pcb_t *running_old = ProcListRunningGet(e_proc_list);
    if (!running_old) {
        TracePrintf(1, "[TrapClock] e_proc_list returned no running process\n");
        Halt();
    }
    memcpy(running_old->uctxt, _uctxt, sizeof(UserContext));

    // 3. Add the old running process to our ready queue
    ProcListReadyAdd(e_proc_list, running_old);
    ProcListReadyPrint(e_proc_list);

    // 4. Get the next process from our ready queue and write its user context to the address
    //    passed in by the yalnix system (this is where it looks to find the context for the
    //    process that it should be executing).
    //
    //    TODO: Eventually, this should just run the DoIdle process if no other processes
    //          are ready. Thus, you may consider *not* adding DoIdle to the ready list.
    //          Instead, since you know it is always pid 0, you can just get it from the
    //          master process list and run it if ready list returns empty.
    pcb_t *running_new = ProcListReadyNext(e_proc_list);
    if (!running_new) {
        TracePrintf(1, "[TrapClock] e_proc_list returned no ready process\n");
        Halt();
    }
    memcpy(_uctxt, running_new->uctxt, sizeof(UserContext));
    ProcListRunningSet(e_proc_list, running_new);

    // 5. Update the kernel's page table so that its stack pages map to
    //    the correct frames for the new running process.
    int kernel_stack_start_page_num = KERNEL_STACK_BASE >> PAGESHIFT;
    memcpy(&e_kernel_pt[kernel_stack_start_page_num],      // Copy the running_new page entries
           running_new->ks,                                // for its kernel stack into the master
           KERNEL_NUMBER_STACK_FRAMES * sizeof(pte_t));    // kernel page table


    // 6. Tell the CPU where to find the page table for our new running process
    WriteRegister(REG_PTBR1, (unsigned int) running_new->pt);    // pt address
    WriteRegister(REG_PTLR1, (unsigned int) MAX_PT_LEN);         // num entries

    // 7. TODO: KCSwitch here???

    //
    TracePrintf(1, "[TrapClock] running_old->sp: %p\trunning_new->sp: %p\n",
                    running_old->uctxt->sp, running_new->uctxt->sp);
    return 0;
}

/*!
 * \desc               Abort the currently running Yalnix user process but continue running other
 *                     processes. 
 *                     
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapIllegal(UserContext *_uctxt) {
    // TracePrintf a message at level 0, giving the process id of the process and some explanation
    // of the problem. 
    // The exit status reported to the parent process of the aborted process when the parent calls
    // the Wait syscall (as described in Section 3.1) should be the value ERROR.    
    // Wrap some of the functionalities below to a function `abort`?
    // Put the current process to `terminated`
    // Free its resources now or only when its parent calls Wait?
    // KernelContextSwitch to a ready process (does KCS need special treatment with an aborted
    // process?) 
    // If there's no process in the ready queue, switch to `idle`
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }
    TracePrintf(1, "[TrapIllegal] _uctxt->sp: %p\n", _uctxt->sp);
    return 0;
}

/*!
 * \desc               This exception results from a disallowed memory access by the current user
 *                     process.
 *                     
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapMemory(UserContext *_uctxt) {
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }
    TracePrintf(1, "[TrapMemory] _uctxt->sp: %p\n", _uctxt->sp);

    // Use the `code` field in `_uctxt` to check what caused this memory trap:
        // YALNIX_MAPERR: Is it because the address is not mapped in the current page tables?
        // YALNIX_ACCERR: or because the access violates the page protection specified in the
        //                corresponding page table entry?
        // OTHERWISE: Is the address outside the virtual address range of the hardware (outside
        //            Region 0 and Region 1)?
    return 0;
}

/*!
 * \desc               This exception results from any arithmetic error from an instruction
 *                     executed by the current user process, such as division by zero or an
 *                     arithmetic overflow.*                     
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapMath(UserContext *_uctxt) {
    // TracePrintf the error message identified with the `code` field in `_uctxt`
    // Refer to `TrapIllegal` above: `Abort` the process and switch to a process in the
    // ready_queue
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }
    TracePrintf(1, "[TrapMath] _uctxt->sp: %p\n", _uctxt->sp);
    return 0;
}


/*!
 * \desc               Reads input from the terminal using TtyReceive and buffers the input line
 *                     for subsequent TtyRead syscalls.
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapTTYReceive(UserContext *_uctxt) {
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }

    // FEEDBACK: What if someone was blocked waiting for this to conclude?
    // 2. Page 25. states that this gets called once there is input ready for a given tty device
    //    Furthermore, page 36 states that the id of the tty device will be in the "code" field.
    TracePrintf(1, "[TrapTTYReceive] _uctxt->sp: %p\n", _uctxt->sp);
    return 0;
}


/*!
 * \desc               Unblock the process that started the TtyWrite, and start the next terminal
 *                     output if there is any.
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapTTYTransmit(UserContext *_uctxt) {
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }

    // FEEDBACK: What if someone was blocked waiting for this to conclude?
    // 2. From my understanding, the steps leading up to this handler getting called are:
    //        - process calls TtyWrite
    //        - TtyWrite calls SyscallTtyWrite
    //        - SyscallTtyWrite calls TtyTransmit
    //        - TtyTransmit generates a trap when finishes which calls this function
    //
    //    At this point, I need to somehow unblock the process and indicate that its write
    //    has finished so that (1) the process can move on or (2) the SyscallTtyWrite can
    //    write more if there are bytes remaining in buf. How do I do this?
    TracePrintf(1, "[TrapTTYTransmit] _uctxt->sp: %p\n", _uctxt->sp);
    return 0;
}


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapDisk(UserContext *_uctxt) {
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }
    TracePrintf(1, "[TrapDisk] _uctxt->sp: %p\n", _uctxt->sp);
    return 0;
}

int TrapNotHandled(UserContext *_uctxt) {
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }
    TracePrintf(1, "[TrapNotHandled] _uctxt->sp: %p\n", _uctxt->sp);
    return 0;
}
