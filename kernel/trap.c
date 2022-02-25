#include <hardware.h>
#include <yalnix.h>
#include <ykernel.h>
#include <ylib.h>

#include "frame.h"
#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "syscall.h"
#include "trap.h"


/*!
 * \desc              Calls the appropriate internel syscall function based on the current process
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            0 on success, ERROR otherwise.
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
 * \desc              This handler gets called every time the clock interrupt fires---the time
 *                    between clock interrupts is the amount of time (i.e., quantum) that a process
 *                    gets to run for before being switched out. Before switching out the current
 *                    process, this handler updates our delay list (i.e., moves processes to the
 *                    ready list if they hit their delay value) and saves the current process'
 *                    UserContext in its pcb for future use.
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            0 on success, ERROR otherwise.
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
    SchedulerUpdateDelay(e_scheduler);
    SchedulerPrintDelay(e_scheduler);

    // 3. Get the pcb for the current running process and the next process to run. If there is
    //    not process in the ready queue, simply return and don't bother context switching.
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[TrapClock] e_scheduler returned no running process\n");
        Halt();
    }

    // 4. Save the UserContext for the current running process in its pcb and add it to the ready
    //    list. Then call our context switch function to switch to the next ready process.
    memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
    SchedulerAddReady(e_scheduler, running_old);
    SchedulerPrintReady(e_scheduler);
    return KCSwitch(_uctxt, running_old);
}


/*!
 * \desc              This trap handler gets called when a process executes an illegal instruction
 *                    (how they would do that, I do not know). We handle this by simply printing
 *                    some debug information on the illegal instruction and exiting the process. 
 *                     
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            We do not return on success (because we kill and switch), ERROR otherwise.
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
    SyscallExit(_uctxt, _uctxt->code);
    return 0;
}


/*!
 * \desc              This handler gets called when the process executes an illegal memory access,
 *                    which may be due to (1) violating page protection permissions (2) accessing
 *                    outside of the processes allowed memory or (3) accessing allowed but unmapped
 *                    memory (i.e., when the stack grows into an unmapped page). For the first two
 *                    cases, we simply print debug information and exit the process. For the third,
 *                    we need to allocate space to allow the stack to grow.
 *                     
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            0 on success, otherwise we do not return (because we kill and switch).
 */
int TrapMemory(UserContext *_uctxt) {
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }

    // 2. If the memory fault was due to invalid permissions, simply abort the process.
    if (_uctxt->code == YALNIX_ACCERR) {
        TracePrintf(1, "[TrapMemory] Invalid permissions: %p\n", _uctxt->addr);
        SyscallExit(_uctxt, ERROR);
    }

    // 3. Grab the pcb for the current running process
    pcb_t *running_old = SchedulerGetRunning(e_scheduler);
    if (!running_old) {
        TracePrintf(1, "[TrapClock] e_scheduler returned no running process\n");
        Halt();
    }

    // 4. If the fault was not due to invalid permissions, then its due to the address pointing
    //    to an unmapped page. Check to see if this unmapped page is between the heap and stack
    //    (i.e., if the fault is due to the stack growing).
    //
    //    Calculate the page number for the address that caused the fault, the brk (but add
    //    a page for the redzone buffer), and then find the current last valid stack page.
    int addr_pn = PTEAddressToPage(_uctxt->addr)     - MAX_PT_LEN;
    int brk_pn  = PTEAddressToPage(running_old->brk) - MAX_PT_LEN + 1;
    int sp_pn   = 0;
    for (int i = addr_pn; i < MAX_PT_LEN; i++) {
        if (running_old->pt[i].valid) {
            sp_pn = i;
            break;
        }
    }

    // 5. Check to see if the unmapped page the process is trying to touch is below
    //    the brk or above the stack. If so, this is not valid so abort the process.
    if (addr_pn < brk_pn || addr_pn > sp_pn) {
        TracePrintf(1, "[TrapMemory] Address out of bounds: %p\n", _uctxt->addr);
        SyscallExit(_uctxt, ERROR);        
    }

    // 6. Find free frames to grow the stack so that the address the process is trying to use
    //    is valid. If we run out of memory, print a message and abort the process. Remember
    //    to flush the TLB afterwards and to update the saved sp in the process' pcb.
    TracePrintf(1, "[TrapMemory] Growing process: %d stack.\n", running_old->pid);
    for (int start = addr_pn; start < sp_pn; start++) {
        int pfn = FrameFindAndSet();
        if (pfn == ERROR) {
            TracePrintf(1, "[TrapMemory] Failed to find a free frame.\n");
            SyscallExit(_uctxt, ERROR);
        }
        TracePrintf(1, "[TrapMemory] Mapping page: %d to frame: %d\n", start, pfn);
        PTESet(running_old->pt, start, PROT_READ | PROT_WRITE, pfn);
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
    memcpy(&running_old->uctxt, _uctxt, sizeof(UserContext));
    return SUCCESS;
}


/*!
 * \desc              This trap handler gets called when a process executes an illegal math
 *                    instruction (such as divide by zero). We handle this by simply printing some
 *                    debug information on the illegal instruction and exiting the process.
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            We do not return on success (because we kill and switch), ERROR otherwise.
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
    SyscallExit(_uctxt, _uctxt->code);
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
 * \desc              Unblock the process that started the TtyWrite, and start the next terminal
 *                    output if there is any.
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            0 on success, ERROR otherwise.
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
 * \desc              These requests are ignored (for now)
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            0 on success, ERROR otherwise.
 */
int TrapDisk(UserContext *_uctxt) {
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }
    TracePrintf(1, "[TrapDisk] _uctxt->sp: %p\t_uctxt->code: %d\n", _uctxt->sp, _uctxt->code);
    return 0;
}


/*!
 * \desc              This handler is for 8 trap handlers that are not handled. It simply prints
 *                    some debugging information and returns.
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            0 on success, ERROR otherwise.
 */
int TrapNotHandled(UserContext *_uctxt) {
    // 1. Check arguments. Return error if invalid.
    if (!_uctxt) {
        return ERROR;
    }
    TracePrintf(1, "[TrapNotHandled] _uctxt->sp: %p\t_uctxt->code: %d\n", _uctxt->sp, _uctxt->code);
    return 0;
}