#include <hardware.h>
#include <yalnix.h>
#include <ykernel.h>
#include <ylib.h>

#include "kernel.h"
#include "syscall.h"
#include "trap.h"


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
            TracePrintf(1, "[TrapKernel_Fork] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallFork();
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_EXEC:
            TracePrintf(1, "[TrapKernel_Exec] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallExec((char *)  _uctxt->regs[0],
            //                     (char **) _uctxt->regs[1]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_EXIT:
            TracePrintf(1, "[TrapKernel_Exit] _uctxt->code: %x\n", _uctxt->code);
            // SyscallExit((int ) _uctxt->regs[0]);
            break;

        case YALNIX_WAIT:
            TracePrintf(1, "[TrapKernel_Wait] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallWait((int *) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_GETPID:
            TracePrintf(1, "[TrapKernel_GetPid] _uctxt->code: %x\n", _uctxt->code);
            ret = SyscallGetPid();
            _uctxt->regs[0] = ret;
            break;

        case YALNIX_BRK:
            TracePrintf(1, "[TrapKernel_Brk] _uctxt->code: %x\n", _uctxt->code);
            ret = SyscallBrk((void *) _uctxt->regs[0]);
            _uctxt->regs[0] = ret;
            break;

        case YALNIX_DELAY:
            TracePrintf(1, "[TrapKernel_Delay] _uctxt->code: %x\n", _uctxt->code);
            ret = SyscallDelay(_uctxt, (int ) _uctxt->regs[0]);
            _uctxt->regs[0] = ret;
            break;

        case YALNIX_TTY_READ:
            TracePrintf(1, "[TrapKernel_TTYRead] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallTtyRead((int )   _uctxt->regs[0],
            //                        (void *) _uctxt->regs[1],
            //                        (int)    _uctxt->regs[2]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_TTY_WRITE:
            TracePrintf(1, "[TrapKernel_TTYWrite] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallTtyWrite((int)    _uctxt->regs[0],
            //                         (void *) _uctxt->regs[1],
            //                         (int )   _uctxt->regs[2]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_PIPE_INIT:
            TracePrintf(1, "[TrapKernel_PipeInit] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallPipeInit((int *) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_PIPE_READ:
            TracePrintf(1, "[TrapKernel_PipeRead] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallPipeRead((int)    _uctxt->regs[0],
            //                         (void *) _uctxt->regs[1],
            //                         (int)    _uctxt->regs[2]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_PIPE_WRITE:
            TracePrintf(1, "[TrapKernel_PipeWrite] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallPipeWrite((int)    _uctxt->regs[0],
            //                          (void *) _uctxt->regs[1],
            //                          (int)    _uctxt->regs[2]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_LOCK_INIT:
            TracePrintf(1, "[TrapKernel_LockInit] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallLockInit((int *) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_LOCK_ACQUIRE:
            TracePrintf(1, "[TrapKernel_LockAquire] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallAcquire((int ) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_LOCK_RELEASE:
            TracePrintf(1, "[TrapKernel_LockRelease] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallRelease((int) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_CVAR_INIT:
            TracePrintf(1, "[TrapKernel_CvarInit] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallCvarInit((int *) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_CVAR_SIGNAL:
            TracePrintf(1, "[TrapKernel_CvarSignal] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallCvarSignal((int ) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_CVAR_BROADCAST:
            TracePrintf(1, "[TrapKernel_CvarBroadcast] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallCvarBroadcast((int ) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_CVAR_WAIT:
            TracePrintf(1, "[TrapKernel_CVarWait] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallCvarWait((int ) _uctxt->regs[0],
            //                         _uctxt->regs[1]);
            // _uctxt->regs[0] = ret;
            break;

        case YALNIX_RECLAIM:
            TracePrintf(1, "[TrapKernel_Reclaim] _uctxt->code: %x\n", _uctxt->code);
            // ret = SyscallReclaim((int ) _uctxt->regs[0]);
            // _uctxt->regs[0] = ret;
            break;

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

    //
    ProcListBlockedDelay(e_proc_list);

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

    // 4. Get the next process from our ready queue and mark it as the current running process.
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
    ProcListRunningSet(e_proc_list, running_new);

    // 5. Switch to the new process. If the new process has never been run before, KCSwitch will
    //    first call KCCopy to initialize the KernelContext for the new process and clone the
    //    kernel stack contents of the old process.
    TracePrintf(1, "[TrapClock] running_old->pid: %d\t\trunning_new->pid: %d\n",
                    running_old->pid, running_new->pid);
    if (!running_old == running_new) {
        int ret = KernelContextSwitch(KCSwitch,
                             (void *) running_old,
                             (void *) running_new);
        if (ret < 0) {
            TracePrintf(1, "[TrapClock] Failed to switch to the next process\n");
            Halt();
        }
    }

    // 6. At this point, this code is being run by the *new* process, which means that its
    //    running_new stack variable is "stale" (i.e., running_new contains the pcb for the
    //    process that this new process previously gave up the CPU for). Thus, get the
    //    current running process (i.e., "this" process) and set the outgoing _uctxt.
    running_new = ProcListRunningGet(e_proc_list);
    memcpy(_uctxt, running_new->uctxt, sizeof(UserContext));
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
    pcb_t *running_old = ProcListRunningGet(e_proc_list);
    if (!running_old) {
        TracePrintf(1, "[TrapClock] e_proc_list returned no running process\n");
        Halt();
    }

    if (_uctxt->code == YALNIX_MAPERR) {
        TracePrintf(1, "[TrapMemory] Address not mapped: %p\n", _uctxt->addr);
    }
    if (_uctxt->code == YALNIX_ACCERR) {
        TracePrintf(1, "[TrapMemory] Invalid permissions: %p\n", _uctxt->addr);
    }
    TracePrintf(1, "[TrapMemory] _uctxt->sp: %p\n", _uctxt->sp);
    TracePrintf(1, "[TrapMemory] running->pid: %d\trunning->uctxt->sp: %p\trunning->uctxt->pc: %p\n",
                   running_old->pid, running_old->uctxt->sp, running_old->uctxt->pc);
    Halt();
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
