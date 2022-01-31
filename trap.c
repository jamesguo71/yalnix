#include "trap.h"


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
            ret = internal_Fork();
            context->regs[0] = ret;

        case YALNIX_EXEC:
            ret = internal_Exec(context->regs[0],
                                context->regs[1]);
            context->regs[0] = ret;

        case YALNIX_EXIT:
            ret = internal_Exit(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_WAIT:
            ret = internal_Wait(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_GETPID:
            ret = internal_GetPid();
            context->regs[0] = ret;

        case YALNIX_BRK:
            ret = internal_Brk(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_DELAY:
            ret = internal_Delay(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_TTY_READ:
            ret = internal_TtyRead(context->regs[0],
                                   context->regs[1],
                                   context->regs[2]);
            context->regs[0] = ret;

        case YALNIX_TTY_WRITE:
            ret = internal_TtyWrite(context->regs[0],
                                    context->regs[1],
                                    context->regs[2]);
            context->regs[0] = ret;

        case YALNIX_PIPE_INIT:
            ret = internal_PipeInit(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_PIPE_READ:
            ret = internal_PipeRead(context->regs[0],
                                    context->regs[1],
                                    context->regs[2]);
            context->regs[0] = ret;

        case YALNIX_PIPE_WRITE:
            ret = internal_PipeWrite(context->regs[0],
                                     context->regs[1],
                                     context->regs[2]);
            context->regs[0] = ret;

        case YALNIX_LOCK_INIT:
            ret = internal_LockInit(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_LOCK_ACQUIRE:
            ret = internal_Acquire(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_LOCK_RELEASE:
            ret = internal_Release(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_CVAR_INIT:
            ret = internal_CvarInit(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_CVAR_SIGNAL:
            ret = internal_CvarSignal(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_CVAR_BROADCAST:
            ret = internal_CvarBroadcast(context->regs[0]);
            context->regs[0] = ret;

        case YALNIX_CVAR_WAIT:
            ret = internal_CvarWait(context->regs[0],
                                    context->regs[1]);
            context->regs[0] = ret;

        case YALNIX_RECLAIM:
            ret = internal_Reclaim(context->regs[0]);
            context->regs[0] = ret;

        default: break;
    }
    return 0;
}

int trap_clock(UserContext *context) {
    return 0;
}

int trap_illegal(UserContext *context) {
    return 0;
}

int trap_memory(UserContext *context) {
    return 0;
}

int trap_math(UserContext *context) {
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

    // 2. I am not sure, but I think we call our internal TtyRead function and, similar to our
    //    trap_kernel, we pass arguments from the process context registers.
    int ret = internal_TtyRead(context->regs[0],
                               context->regs[1],
                               context->regs[2]);
    if (!ret) {
        // what do I do if the call fails?
    }
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

    // 2. I am not sure, but I think we call our internal TtyWrite function and, similar to our
    //    trap_kernel, we pass arguments from the process context registers.
    int ret = internal_TtyWrite(context->regs[0],
                                context->regs[1],
                                context->regs[2]);
    if (!ret) {
        // what do I do if the call fails?
    }
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
    return 0;
}