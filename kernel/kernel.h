#ifndef __KERNEL_H
#define __KERNEL_H
#include <hardware.h>
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
extern lock_list_t *e_lock_list;
extern pipe_list_t *e_pipe_list;
extern pte_t       *e_kernel_pt; // Kernel Page Table
extern scheduler_t *e_scheduler;
extern tty_t       *e_tty;
extern void        *e_kernel_curr_brk;


/*!
 * \desc            Sets the kernel's brk (i.e., the address right above the kernel heap)
 * 
 * \param[in] _brk  The address of the new brk
 * 
 * \return          0 on success, ERROR otherwise.
 */
int SetKernelBrk(void *_brk);


/*!
 * \desc                 TODO
 * 
 * \param[in] cmd_args   TODO
 * \param[in] pmem_size  TODO
 * \param[in] uctxt      TODO
 */
void KernelStart (char **cmd_args, unsigned int pmem_size, UserContext *uctxt);


/*!
 * \desc                 TODO
 * 
 * \param[in] cmd_args   TODO
 * \param[in] pmem_size  TODO
 * \param[in] uctxt      TODO
 */
int KCSwitch(UserContext *_uctxt, pcb_t *_running_old);


/*!
 * \desc                  TODO
 *                       
 *                       
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