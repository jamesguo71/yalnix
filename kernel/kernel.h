#ifndef __KERNEL_H
#define __KERNEL_H
#include <hardware.h>
#include "cvar.h"
#include "lock.h"
#include "pipe.h"
#include "process.h"
#include "scheduler.h"
#include "tty.h"

#define KERNEL_BYTE_SIZE           8
#define KERNEL_NUMBER_STACK_FRAMES KERNEL_STACK_MAXSIZE / PAGESIZE


/*
 * Variable definitions - Declare these globally in kernel.c, but not in trap.c or syscalls.c;
 *                        they should be able to refer to them normally---no need to define or
 *                        declare them in trap.c/h or syscall.c/h
 */
extern char        *e_frames;
extern int          e_num_frames;
extern cvar_list_t *e_cvar_list;
extern lock_list_t *e_lock_list;
extern pipe_list_t *e_pipe_list;
extern pte_t       *e_kernel_pt; // Kernel Page Table
extern scheduler_t *e_scheduler;
extern tty_list_t  *e_tty_list;
extern void        *e_kernel_curr_brk;


/*!
 * \desc            Grows or shrinks the kernel's heap depending on the current and new values of
 *                  brk. This involves finding and mapping new frames to the kernel's page table
 *                  when growing, or releasing frames and invalidating pages when shrinking.
 *                  The TLB is flushed after page table changes to ensure fresh entries.
 * 
 * \param[in] _brk  The address of the new brk (i.e., end of the heap)
 * 
 * \return          0 on success, ERROR otherwise.
 */
int SetKernelBrk(void *_brk);


/*!
 * \desc                  Initializes kernel variables and structures needs to track processes,
 *                        locks, cvars, locks, and other OS related functionalities. Additionally,
 *                        we setup the kernel's page table and a dummy "DoIdle" process to run
 *                        when no other processes are available.
 * 
 * \param[in] _cmd_args   Not used for checkpoint 2
 * \param[in] _pmem_size  The size of the physical memory availabe to our system (in bytes)
 * \param[in] _uctxt      An initialized usercontext struct for the DoIdle process
 */
void KernelStart (char **cmd_args, unsigned int pmem_size, UserContext *uctxt);


/*!
 * \desc                    Small wrapper for context switching to check if the next process we
 *                          are going to switch to is the same as the current process. If so,
 *                          we simply return without performing the context switch. Otherwise, it
 *                          calls MyKCS which updates the kernel stack and TLB to the new process.
 *                          Finally, we set the UserContext to the new process we are going to run.
 * 
 * \param[in] _uctxt        The UserContext for the current running process
 * \param[in] _running_old  The pcb for the current running process
 * 
 * \return                  0 on success, halts otherwise
 */
int KCSwitch(UserContext *_uctxt, pcb_t *_running_old);


/*!
 * \desc                  Saves the KernelContext for the current running process and reconfigures
 *                        the kernel page table and stack context to match those of the new
 *                        process. If the new process has never been run before (i.e., does not
 *                        have an initialized KernelContext) KCCopy is first called.
 * 
 * \param[in] kc_in       a pointer to a temporary copy of the current kernel context of the caller
 * \param[in] curr_pcb_p  a void pointer to current process's pcb
 * \param[in] next_pcb_p  a void pointer to the next process's pcb
 * 
 * \return                return a pointer to a kernel context it had earlier saved in the next
 *                        process’s PCB.
 */
KernelContext *MyKCS(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);


/*!
 * \desc                 Copies the kernel context from kc_in into the new pcb, and copies the
 *                       contents of the current kernel stack into the frames that have been
 *                       allocated for the new process' kernel stack.
 * 
 * \param[in] kc_in      A pointer to the kernel context to be copied
 * \param[in] new_pcb_p  A pointer to the pcb of the new process
 * \param[in] not_used   Not used
 * 
 * \return               Returns the kc_in pointer
 */
KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used);
#endif