#include <hardware.h>
#include <yalnix.h>
#include <ykernel.h>
#include <ylib.h>

#include "kernel.h"
#include "process.h"
#include "scheduler.h"
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
        TracePrintf(1, "[TrapKernel] Invalid _uctxt pointer\n");
        return ERROR;
    }

    // 2. Page. 36 of the manual states that the "code" field in UserContext will contain
    //    the number of the syscal (as defined in yalnix.h). Additionally, any arguments
    //    to the syscall will be placed in the "regs" array beginning with reg[0]. Use
    //    this information to dispatch the appropriate syscall function. Afterwards, store
    //    the syscall return value in reg[0].
    switch(_uctxt->code) {
        case YALNIX_FORK:
             _uctxt->regs[0] = SyscallFork(_uctxt);
            break;

        case YALNIX_EXEC:
            _uctxt->regs[0] = SyscallExec(_uctxt,
                                (char *)  _uctxt->regs[0],
                                (char **) _uctxt->regs[1]);
            break;

        case YALNIX_EXIT:
            SyscallExit(_uctxt, (int ) _uctxt->regs[0]);
            break;

        case YALNIX_WAIT:
            _uctxt->regs[0] = SyscallWait(_uctxt, (int *) _uctxt->regs[0]);
            break;

        case YALNIX_GETPID:
            _uctxt->regs[0] = SyscallGetPid();
            break;

        case YALNIX_BRK:
            _uctxt->regs[0] = SyscallBrk(_uctxt, (void *) _uctxt->regs[0]);
            break;

        case YALNIX_DELAY:
            _uctxt->regs[0] = SyscallDelay(_uctxt, (int ) _uctxt->regs[0]);
            break;

        case YALNIX_TTY_READ:
            _uctxt->regs[0] = SyscallTtyRead(_uctxt,
                                    (int )   _uctxt->regs[0],
                                    (void *) _uctxt->regs[1],
                                    (int)    _uctxt->regs[2]);
            break;

        case YALNIX_TTY_WRITE:
            _uctxt->regs[0] = SyscallTtyWrite(_uctxt,
                                     (int)    _uctxt->regs[0],
                                     (void *) _uctxt->regs[1],
                                     (int)    _uctxt->regs[2]);
            break;

        case YALNIX_PIPE_INIT:
            _uctxt->regs[0] = SyscallPipeInit((int *) _uctxt->regs[0]);
            break;

        case YALNIX_PIPE_READ:
            _uctxt->regs[0] = SyscallPipeRead((int)    _uctxt->regs[0],
                                              (void *) _uctxt->regs[1],
                                              (int)    _uctxt->regs[2]);
            break;

        case YALNIX_PIPE_WRITE:
            _uctxt->regs[0] = SyscallPipeWrite((int)    _uctxt->regs[0],
                                               (void *) _uctxt->regs[1],
                                               (int)    _uctxt->regs[2]);
            break;

        case YALNIX_LOCK_INIT:
            _uctxt->regs[0] = SyscallLockInit((int *) _uctxt->regs[0]);
            break;

        case YALNIX_LOCK_ACQUIRE:
            _uctxt->regs[0] = SyscallAcquire((int ) _uctxt->regs[0]);
            break;

        case YALNIX_LOCK_RELEASE:
            _uctxt->regs[0] = SyscallRelease((int) _uctxt->regs[0]);
            break;

        case YALNIX_CVAR_INIT:
            _uctxt->regs[0] = SyscallCvarInit((int *) _uctxt->regs[0]);
            break;

        case YALNIX_CVAR_SIGNAL:
            _uctxt->regs[0] = SyscallCvarSignal((int ) _uctxt->regs[0]);
            break;

        case YALNIX_CVAR_BROADCAST:
            _uctxt->regs[0] = SyscallCvarBroadcast((int ) _uctxt->regs[0]);
            break;

        case YALNIX_CVAR_WAIT:
            _uctxt->regs[0] = SyscallCvarWait((int ) _uctxt->regs[0],
                                                     _uctxt->regs[1]);
            break;

        case YALNIX_RECLAIM:
            _uctxt->regs[0] = SyscallReclaim((int ) _uctxt->regs[0]);
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

    // 2. Update any processes that are currently blocked due to a delay call. This will
    //    loop over blocked list and decrement the clock_count for any processes delaying.
    //    If their count hits zero, they get added to the ready queue.
    //    TODO: Better name?
    SchedulerUpdateDelay(e_scheduler);
    SchedulerPrintDelay(e_scheduler);

    // 3. Get the pcb for the current running process and the next process to run. If there is
    //    not process in the ready queue, simply return and don't bother context switching.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[TrapClock] e_scheduler returned no running process\n");
        Halt();
    }
    memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
    SchedulerAddReady(e_scheduler, running_old);
    SchedulerPrintReady(e_scheduler);
    return KCSwitch(_uctxt, running_old);
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
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }

    // 2. Grab the current running process and print some debug information. Then kill the
    //    process by calling SyscallExit with the illegal instruction as the exit code.
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    TracePrintf(1, "[TrapIllegal] Killing process: %d for illegal instruction: %d\n",
                                  running->pid, _uctxt->code);
    SyscallExit(_uctxt, uctxt->code);
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
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[TrapClock] e_scheduler returned no running process\n");
        Halt();
    }

    if (_uctxt->code == YALNIX_MAPERR) {
        TracePrintf(1, "[TrapMemory] Address not mapped: %p\n", _uctxt->addr);
    }
    if (_uctxt->code == YALNIX_ACCERR) {
        TracePrintf(1, "[TrapMemory] Invalid permissions: %p\n", _uctxt->addr);
    }
    TracePrintf(1, "[TrapMemory] _uctxt->sp: %p\n", _uctxt->sp);
    TracePrintf(1, "[TrapMemory] running->pid: %d\trunning->uctxt.sp: %p\trunning->uctxt.pc: %p\n",
                   running_old->pid, running_old->uctxt.sp, running_old->uctxt.pc);
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
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }

    // 2. Grab the current running process and print some debug information. Then kill the
    //    process by calling SyscallExit with the math error as the exit code.
    pcb_t *running = SchedulerGetRunning(e_scheduler);
    TracePrintf(1, "[TrapMath] Killing process: %d for math error: %d\n",
                                  running->pid, _uctxt->code);
    SyscallExit(_uctxt, uctxt->code);
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

    // 3. Read input from terminal (terminal id is in "code" field of uctxt) into the kernel's
    //    ttyread buffer. Then check to see if there are any processes blocked on TTYRead for
    //    the specific tty device. If so, remove them from blocked and add to ready.
    //
    // code here for copying terminal contents to kernel buffer...
    // SchedulerUpdateTTYRead();
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

    // 2. Check to see if there is a process blocked on TTYWrite for this device. If so,
    //    remove them from blocked and add to ready queue.
    TracePrintf(1, "[TrapTTYTransmit] _uctxt->sp: %p\n", _uctxt->sp);
    // SchedulerUpdateTTYWrite();
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