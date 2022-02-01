#ifndef __KERNEL_H
#define __KERNEL_H


/*
 * Struct definitions - transparent to any file that includes "kernel.h". Do we want to make
                        them opaque?
 */
typedef struct pcb {
    int  pid;
    int  brk;
    int  exit_status;   // for saving the process's exit status, See Page 32
    int  exited;        // if the process has exited?  
    int *held_locks;    // locks held by the current process, used by sync syscalls

    struct pcb *parent;     // For keeping track of parent process
    struct pcb *children;   // For keeping track of children processes

    UserContext   *uctxt; // Defined in `hardware.h`    
    KernelContext *kctxt;    // Needed for KernelCopy? See Page 45 

    pte_t *ks_frames;
    pte_t *pt;              // Defined in `hardware.h`
} pcb_t;

typedef struct lock {
    int  id;     
    int  value;     // FREE or LOCKED?
    int  owner;     // which process currently owns the lock?
    int *waiting;   // which processes are waiting for the lock?
} lock_t;

typedef struct cvar {
    int  id;
    int *waiting;   // which processes are waiting for the cvar?
} cvar_t;


/*
 * Variable definitions - I *think* extern keyword allows us to define these variables such that
 *                        any file that includes "kernel.h" may use them. If we don't need to
 *                        export them, however, move to kernel.c so that they remain hidden.
 */
extern int g_ready_len;
extern int g_blocked_len;
extern int g_terminated_len;
extern pcb_t *g_current;
extern pcb_t *g_ready;
extern pcb_t *g_blocked;
extern pcb_t *g_terminated;
extern void  *g_kernel_curr_brk;


/*!
 * \desc            Sets the kernel's brk (i.e., the address right above the kernel heap)
 * 
 * \param[in] _brk  The address of the new brk
 * 
 * \return          0 on success, ERROR otherwise.
 */
int SetKernelBrk(void *_brk);
void KernelStart (char **cmd_args, unsigned int pmem_size, UserContext *uctxt);
KernelContext *MyKCS(KernelContext *, void *, void *);


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