#ifndef __KERNEL_H
#define __KERNEL_H
#include "hardware.h"

/*
 * Struct definitions - transparent to any file that includes "kernel.h". Do we want to make
                        them opaque?
 */
typedef struct cvar {
    int  id;
    int *waiting;   // which processes are waiting for the cvar?
} cvar_t;

typedef struct lock {
    int  id;     
    int  owner;     // which process currently owns the lock?
    int *waiting;   // which processes are waiting for the lock?
} lock_t;

typedef struct pcb {
    int  pid;
    int  brk;
    int  exit_status;       // for saving the process's exit status, See Page 32
    int  exited;            // if the process has exited?  
    int *held_locks;        // locks held by the current process, used by sync syscalls

    struct pcb *parent;     // For keeping track of parent process
    struct pcb *children;   // For keeping track of children processes

    UserContext   *uctxt;   // Defined in `hardware.h`    
    KernelContext *kctxt;   // Needed for KernelCopy? See Page 45 

    pte_t *ks_frames;
    pte_t *pt;              // Defined in `hardware.h`
} pcb_t;

typedef struct pipe {
    int   id;
    int   plen;
    int   read;
    int   write;
    void *buf;
} pipe_t;

/*
 * Variable definitions - Declare these globally in kernel.c, but not in trap.c or syscalls.c;
                          they should be able to refer to them normally---no need to define or
                          declare them in trap.c/h or syscall.c/h
 */
extern int g_cvars_len;
extern int g_locks_len;
extern int g_pipes_len;
extern int g_ready_len;
extern int g_blocked_len;
extern int g_terminated_len;

// Used by TTY traps to indicate when Receive/Write hardware operations complete.
// Initialize to hold NUM_TERMINALS ints (i.e., a flag for each tty device)
extern int *g_tty_read_ready;
extern int *g_tty_write_ready;

// Used to track locks and cvars. Is array best way to track these? We will have to
// realloc everytime we make a new one. Maybe add "next" pointer to lock/cvar struct?
extern cvar_t *g_cvars;
extern lock_t *g_locks;
extern pipe_t *g_pipes;

extern pcb_t *g_current;
extern pcb_t *g_ready;
extern pcb_t *g_blocked;
extern pcb_t *g_terminated;

// Used to determine if the original brk has changed - initialized to og brk value
extern void *g_kernel_curr_brk;

// I *think* I need these for storing results of tty read/writes since TtyReceive/Transmit require
// that the buf reside in kernel memory. Initialize to hold NUM_TERMINALS void pointers (i.e., a
// pointer for each tty device.). Do I need length variables for these too?
extern void **g_tty_read_buf;
extern void **g_tty_write_buf;


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