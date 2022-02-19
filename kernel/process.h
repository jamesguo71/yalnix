#ifndef __PROCESS_H
#define __PROCESS_H
#include <hardware.h>


// TODO: Add a char *name field for debugging/readability?
typedef struct pcb {
    int  pid;
    int  clock_ticks;
    int  exit_status;       // for saving the process's exit status, See Page 32
    int  exited;            // if the process has exited?  
    int *held_locks;        // locks held by the current process, used by sync syscalls

    struct pcb *parent;     // For keeping track of parent process
    struct pcb *children;   // For keeping track of children processes
    struct pcb *sibling;

    KernelContext *kctxt;   // Needed for KernelCopy? See Page 45 
    UserContext   *uctxt;   // Defined in `hardware.h`    

    pte_t *ks;
    pte_t *pt;              // Defined in `hardware.h`

    void *brk;
    void *data_end;
} pcb_t;


pcb_t *ProcessCreate();
pcb_t *ProcessCreateIdle();
int    ProcessDelete();
int    ProcessTerminate();
#endif // __PROCESS_H