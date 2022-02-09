#include "trap.h"
#include "hardware.h"
#include "syscall.h"
#include "yalnix.h"
#include "ylib.h"

/*!
 * \desc               Calls the appropriate internel syscall function based on the current process
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_kernel(UserContext *context) {
    // 1. Check arguments. Return error if invalid.
    if (!context) {
        return ERROR;
    }

    // 2. Page. 36 of the manual states that the "code" field in UserContext will contain
    //    the number of the syscal (as defined in yalnix.h). Additionally, any arguments
    //    to the syscall will be placed in the "regs" array beginning with reg[0]. Use
    //    this information to dispatch the appropriate syscall function. Afterwards, store
    //    the syscall return value in reg[0].
    u_long ret = 0;
    switch(context->code) {
        case YALNIX_FORK:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_Fork();
            // context->regs[0] = ret;

        case YALNIX_EXEC:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_Exec((char *)  context->regs[0],
            //                     (char **) context->regs[1]);
            // context->regs[0] = ret;

        case YALNIX_EXIT:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // internal_Exit((int ) context->regs[0]);

        case YALNIX_WAIT:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_Wait((int *) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_GETPID:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_GetPid();
            // context->regs[0] = ret;

        case YALNIX_BRK:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_Brk((void *) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_DELAY:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_Delay((int ) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_TTY_READ:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_TtyRead((int )   context->regs[0],
            //                        (void *) context->regs[1],
            //                        (int)    context->regs[2]);
            // context->regs[0] = ret;

        case YALNIX_TTY_WRITE:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_TtyWrite((int)    context->regs[0],
            //                         (void *) context->regs[1],
            //                         (int )   context->regs[2]);
            // context->regs[0] = ret;

        case YALNIX_PIPE_INIT:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_PipeInit((int *) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_PIPE_READ:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_PipeRead((int)    context->regs[0],
            //                         (void *) context->regs[1],
            //                         (int)    context->regs[2]);
            // context->regs[0] = ret;

        case YALNIX_PIPE_WRITE:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_PipeWrite((int)    context->regs[0],
            //                          (void *) context->regs[1],
            //                          (int)    context->regs[2]);
            // context->regs[0] = ret;

        case YALNIX_LOCK_INIT:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_LockInit((int *) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_LOCK_ACQUIRE:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_Acquire((int ) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_LOCK_RELEASE:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_Release((int) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_CVAR_INIT:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_CvarInit((int *) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_CVAR_SIGNAL:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_CvarSignal((int ) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_CVAR_BROADCAST:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_CvarBroadcast((int ) context->regs[0]);
            // context->regs[0] = ret;

        case YALNIX_CVAR_WAIT:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_CvarWait((int ) context->regs[0],
            //                         context->regs[1]);
            // context->regs[0] = ret;

        case YALNIX_RECLAIM:
            TracePrintf(1, "[trap_kernel] context->code: %x\n", context->code);
            // ret = internal_Reclaim((int ) context->regs[0]);
            // context->regs[0] = ret;

        default: break;
    }
    return 0;
}

/*!
 * \desc               If there are other runnable processes on the ready queue, perform a context switch to the next runnable process. 
 *                     
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_clock(UserContext *context) {    
    // traceprints when there’s a clock trap.    
    // (The Yalnix kernel should implement round-robin process scheduling with a CPU quantum per process of 1 clock tick.)
    // Check if there're processes on the ready queue
        // If so, KernelContextSwitch to it
        // Otherwise, dispatch idle.
    // 1. Check arguments. Return error if invalid.
    if (!context) {
        return ERROR;
    }
    TracePrintf(1, "[trap_clock] context->sp: %p\n", context->sp);
    return 0;
}

/*!
 * \desc               Abort the currently running Yalnix user process but continue running other processes. 
 *                     
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_illegal(UserContext *context) {
    // TracePrintf a message at level 0, giving the process id of the process and some explanation of the problem. 
    // The exit status reported to the parent process of the aborted process when the parent calls the Wait syscall (as described in Section 3.1) should be the value ERROR.    
    // Wrap some of the functionalities below to a function `abort`?
    // Put the current process to `terminated`
    // Free its resources now or only when its parent calls Wait?
    // KernelContextSwitch to a ready process (does KCS need special treatment with an aborted process?) 
    // If there's no process in the ready queue, switch to `idle`
    // 1. Check arguments. Return error if invalid.
    if (!context) {
        return ERROR;
    }
    TracePrintf(1, "[trap_illegal] context->sp: %p\n", context->sp);
    return 0;
}

/*!
 * \desc               This exception results from a disallowed memory access by the current user process.
 *                     
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_memory(UserContext *context) {
    // Use the `code` field in `context` to check what caused this memory trap:
        // YALNIX_MAPERR: Is it because the address is not mapped in the current page tables?
        // YALNIX_ACCERR: or because the access violates the page protection specified in the corresponding page table entry?
        // OTHERWISE: Is the address outside the virtual address range of the hardware (outside Region 0 and Region 1)?
    // 1. Check arguments. Return error if invalid.
    if (!context) {
        return ERROR;
    }
    TracePrintf(1, "[trap_memory] context->sp: %p\n", context->sp);
    return 0;
}

/*!
 * \desc               This exception results from any arithmetic error from an instruction executed by the
 *                     current user process, such as division by zero or an arithmetic overflow.*                     
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_math(UserContext *context) {
    // TracePrintf the error message identified with the `code` field in `context`
    // Refer to `trap_illegal` above: `Abort` the process and switch to a process in the ready_queue
    // 1. Check arguments. Return error if invalid.
    if (!context) {
        return ERROR;
    }
    TracePrintf(1, "[trap_math] context->sp: %p\n", context->sp);
    return 0;
}


/*!
 * \desc               Reads input from the terminal using TtyReceive and buffers the input line
 *                     for subsequent TtyRead syscalls.
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_tty_receive(UserContext *context) {
    // 1. Check arguments. Return error if invalid.
    if (!context) {
        return ERROR;
    }

    // // 2. Page 25. states that this gets called once there is input ready for a given tty device.
    // //    Furthermore, page 36 states that the id of the tty device will be in the "code" field.
    // g_tty_read_ready[context->code] = 1;
    TracePrintf(1, "[trap_tty_receive] context->sp: %p\n", context->sp);
    return 0;
}


/*!
 * \desc               Unblock the process that started the TtyWrite, and start the next terminal
 *                     output if there is any.
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_tty_transmit(UserContext *context) {
    // 1. Check arguments. Return error if invalid.
    if (!context) {
        return ERROR;
    }

    // // 2. From my understanding, the steps leading up to this handler getting called are:
    // //        - process calls TtyWrite
    // //        - TtyWrite calls internal_TtyWrite
    // //        - internal_TtyWrite calls TtyTransmit
    // //        - TtyTransmit generates a trap when finishes which calls this function
    // //
    // //    At this point, I need to somehow unblock the process and indicate that its write
    // //    has finished so that (1) the process can move on or (2) the internal_TtyWrite can
    // //    write more if there are bytes remaining in buf. How do I do this?
    // g_tty_write_ready[context->code] = 1;
    TracePrintf(1, "[trap_tty_transmit] context->sp: %p\n", context->sp);
    return 0;
}


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_disk(UserContext *context) {
    // 1. Check arguments. Return error if invalid.
    if (!context) {
        return ERROR;
    }
    TracePrintf(1, "[trap_disk] context->sp: %p\n", context->sp);
    return 0;
}

int trap_not_handled(UserContext *context) {
    // 1. Check arguments. Return error if invalid.
    if (!context) {
        return ERROR;
    }
    TracePrintf(1, "[trap_not_handled] context->sp: %p\n", context->sp);
    return 0;
}