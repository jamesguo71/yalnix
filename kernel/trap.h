#ifndef __TRAP_H
#define __TRAP_H
#include <hardware.h>


/*!
 * \desc               Calls the appropriate internel syscall function based on the current process
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapKernel(UserContext *_uctxt);


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
int TrapClock(UserContext *_uctxt);


/*!
 * \desc              This trap handler gets called when a process executes an illegal instruction
 *                    (how they would do that, I do not know). We handle this by simply printing
 *                    some debug information on the illegal instruction and exiting the process. 
 *                     
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            We do not return on success (because we kill and switch), ERROR otherwise.
 */
int TrapIllegal(UserContext *_uctxt);


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
int TrapMemory(UserContext *_uctxt);


/*!
 * \desc              This trap handler gets called when a process executes an illegal math
 *                    instruction (such as divide by zero). We handle this by simply printing some
 *                    debug information on the illegal instruction and exiting the process.
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            We do not return on success (because we kill and switch), ERROR otherwise.
 */
int TrapMath(UserContext *_uctxt);


/*!
 * \desc              This handler gets called when there is input waiting to be read from one
 *                    of the terminals. It calls our TTYUpdateReader function, which reads and
 *                    saves the input into kernel memory for future use by processes.
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return            0 on success, ERROR otherwise.
 */
int TrapTTYReceive(UserContext *_uctxt);


/*!
 * \desc               Unblock the process that started the TtyWrite, and start the next terminal
 *                     output if there is any.
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapTTYTransmit(UserContext *_uctxt);


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapDisk(UserContext *_uctxt);


/*!
 * \desc               These requests are ignored (for now)
 * 
 * \param[in] _uctxt  The UserContext for the process associated with the TRAP
 * 
 * \return             0 on success, ERROR otherwise.
 */
int TrapNotHandled(UserContext *_uctxt);
#endif
