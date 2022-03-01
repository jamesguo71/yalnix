#include <hardware.h>
#include <yalnix.h>
#include <ykernel.h>
#include <ylib.h>

#include "cvar.h"
#include "frame.h"
#include "lock.h"
#include "kernel.h"
#include "pipe.h"
#include "process.h"
#include "pte.h"
#include "scheduler.h"
#include "syscall.h"
#include "trap.h"
#include "tty.h"


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
            _uctxt->regs[0] = TTYRead(e_tty_list,           // tty struct declared in kernel.h
                                      _uctxt,               // current process' UserContext
                             (int )   _uctxt->regs[0],      // tty device id
                             (void *) _uctxt->regs[1],      // output buffer to store read bytes
                             (int)    _uctxt->regs[2]);     // length of output buffer
            break;
        case YALNIX_TTY_WRITE:
            _uctxt->regs[0] = TTYWrite(e_tty_list,          // tty struct declared in kernel.h
                                       _uctxt,              // current process' UserContext
                              (int)    _uctxt->regs[0],     // tty device id
                              (void *) _uctxt->regs[1],     // input buffer with bytes to write
                              (int)    _uctxt->regs[2]);    // length of input buffer
            break;
        case YALNIX_PIPE_INIT:
            _uctxt->regs[0] = PipeInit(e_pipe_list,      // pipe_list struct declared in kernel.h 
                               (int *) _uctxt->regs[0]); // pointer to store new pipe id
            break;
        case YALNIX_PIPE_READ:
            _uctxt->regs[0] = PipeRead(e_pipe_list,       // pipe_list struct declared in kernel.h
                                       _uctxt,            // current process' UserContext
                              (int)    _uctxt->regs[0],   // pipe id
                              (void *) _uctxt->regs[1],   // output buffer to store read bytes
                              (int)    _uctxt->regs[2]);  // length of output buffer
            break;
        case YALNIX_PIPE_WRITE:
            _uctxt->regs[0] = PipeWrite(e_pipe_list,      // pipe_list struct declared in kernel.h
                                        _uctxt,           // current process' UserContext
                               (int)    _uctxt->regs[0],  // pipe id
                               (void *) _uctxt->regs[1],  // input buffer with bytes to write
                               (int)    _uctxt->regs[2]); // length of input buffer
            break;
        case YALNIX_LOCK_INIT:
            _uctxt->regs[0] = LockInit(e_lock_list,       // lock_list struct declared in kernel.h
                               (int *) _uctxt->regs[0]);  // pointer to store new lock id
            break;
        case YALNIX_LOCK_ACQUIRE:
            _uctxt->regs[0] = LockAcquire(e_lock_list,      // lock_list struct declared in kernel.h
                                          _uctxt,           // current process' UserContext
                                   (int ) _uctxt->regs[0]); // lock id
            break;
        case YALNIX_LOCK_RELEASE:
            _uctxt->regs[0] = LockRelease(e_lock_list,      // lock_list struct declared in kernel.h
                                    (int) _uctxt->regs[0]); // lock id
            break;
        case YALNIX_CVAR_INIT:
            _uctxt->regs[0] = CVarInit(e_cvar_list,           // cvar_list struct
                               (int *) _uctxt->regs[0]);      // pointer to store new cvar id
            break;
        case YALNIX_CVAR_SIGNAL:
            _uctxt->regs[0] = CVarSignal(e_cvar_list,         // cvar_list struct
                                  (int ) _uctxt->regs[0]);    // cvar id
            break;
        case YALNIX_CVAR_BROADCAST:
            _uctxt->regs[0] = CVarBroadcast(e_cvar_list,      // cvar_list struct
                                     (int ) _uctxt->regs[0]); // cvar id
            break;
        case YALNIX_CVAR_WAIT:
            _uctxt->regs[0] = CVarWait(e_cvar_list,           // cvar_list struct
                                       _uctxt,                // current process' UserContext
                                (int ) _uctxt->regs[0],       // cvar id
                                (int ) _uctxt->regs[1]);      // lock id
            break;
        case YALNIX_RECLAIM:
            if (_uctxt->regs[0] < LOCK_ID_START) {              // id order is cvar < lock < pipe 
                _uctxt->regs[0] = CVarReclaim(e_cvar_list,      // so if the id < lock_start then
                                              _uctxt->regs[0]); // it must be cvar. Follow this
            } else if (_uctxt->regs[0] < PIPE_ID_START) {       // pattern to determine if the id
                _uctxt->regs[0] = LockReclaim(e_lock_list,      // is for a cvar, lock, or pipe
                                              _uctxt->regs[0]); // struct and call the appropriate
            } else {                                            // reclaim function
                _uctxt->regs[0] = PipeReclaim(e_pipe_list,
                                              _uctxt->regs[0]);
            }
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

    // 2. Page 25. states that this gets called once there is input ready for a given tty device
    //    Furthermore, page 36 states that the id of the tty device will be in the "code" field.
    TTYUpdateReader(e_tty_list, _uctxt->code);
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
    TTYUpdateWriter(e_tty_list, _uctxt, _uctxt->code);
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