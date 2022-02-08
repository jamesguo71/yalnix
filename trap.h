#ifndef __TRAP_H
#define __TRAP_H
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
int trap_kernel(UserContext *context);


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_clock(UserContext *context);


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_illegal(UserContext *context);


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_memory(UserContext *context);


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_math(UserContext *context);


/*!
 * \desc               Reads input from the terminal using TtyReceive and buffers the input line
 *                     for subsequent TtyRead syscalls.
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_tty_receive(UserContext *context);


/*!
 * \desc               Unblock the process that started the TtyWrite, and start the next terminal
 *                     output if there is any.
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_tty_transmit(UserContext *context);


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_disk(UserContext *context);


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] context  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int trap_not_handled(UserContext *context);
#endif