#ifndef __PROCESS_H
#define __PROCESS_H
#include <hardware.h>

#define KERNEL_NUMBER_STACK_FRAMES KERNEL_STACK_MAXSIZE / PAGESIZE

// TODO: Add a char *name field for debugging/readability?
typedef struct pcb {
    int  pid;
    int  clock_ticks;
    int  exit_status;       // for saving the process's exit status, See Page 32
    int  exited;            // if the process has exited?
    int  lock_id;
    int  pipe_id;
    int  tty_id;

    struct pcb *parent;     // For keeping track of parent process
    struct pcb *headchild;   // For keeping track of children processes
    struct pcb *sibling;

    KernelContext *kctxt;   // Needed for KernelCopy? See Page 45
    UserContext uctxt;

    pte_t ks[KERNEL_NUMBER_STACK_FRAMES];
    pte_t pt[MAX_PT_LEN];

    void *brk;
    void *data_end;
} pcb_t;


pcb_t *ProcessCreate();
pcb_t *ProcessCreateIdle();

void ProcessTerminate(pcb_t *_process);
void ProcessDelete(pcb_t *_process);
void ProcessDestroy(pcb_t *_process);
void ProcessAddChild(pcb_t *_parent, pcb_t *_child);
void ProcessRemoveChild(pcb_t *_parent, pcb_t *_child);
#endif // __PROCESS_H