#ifndef __PROC_LIST_H
#define __PROC_LIST_H

#include "hardware.h"
#include "kernel.h"



typedef struct proc_list proc_list_t;

typedef struct pcb {
    int  pid;
    int  exit_status;       // for saving the process's exit status, See Page 32
    int  exited;            // if the process has exited?  
    int *held_locks;        // locks held by the current process, used by sync syscalls

    struct pcb *parent;     // For keeping track of parent process
    struct pcb *children;   // For keeping track of children processes

    KernelContext kctxt;   // Needed for KernelCopy? See Page 45
    UserContext   uctxt;   // Defined in `hardware.h`

    pte_t ks[KERNEL_NUMBER_STACK_FRAMES];
    // TODO: We probably can also make this a member structure, just need to bzero its bytes after pcb created
    pte_t pt[MAX_PT_LEN];              // Defined in `hardware.h`

    void *brk;
    void *text_end;
    void *data_end;
} pcb_t;


/*!
 * \desc                   Initializes memory for a new proc_list_t struct
 *
 * \return                 An initialized proc_list_t struct, NULL otherwise.
 */
proc_list_t *ProcListCreate();


/*!
 * \desc              Frees the memory associated with a proc_list_t struct
 *
 * \param[in] bridge  A bridge_t struct that the caller wishes to free
 */
int ProcListDelete(proc_list_t *_proc_list);

//int    ProcListBlockedAdd(proc_list_t *_proc_list, pcb_t *_process);
//pcb_t *ProcListBlockedGet(proc_list_t *_proc_list, int _pid);
//pcb_t *ProcListBlockedNext(proc_list_t *_proc_list);
//int    ProcListBlockedPrint(proc_list_t *_proc_list);
//int    ProcListBlockedRemove(proc_list_t *_proc_list, int _pid);
//
int    ProcListProcessAdd(proc_list_t *_proc_list, pcb_t *_process);
//pcb_t *ProcListProcessGet(proc_list_t *_proc_list, int _pid);
int    ProcListProcessPrint(proc_list_t *_proc_list);
//int    ProcListProcessRemove(proc_list_t *_proc_list, int _pid);
//
int    ProcListReadyAdd(proc_list_t *_proc_list, pcb_t *_process);
//pcb_t *ProcListReadyGet(proc_list_t *_proc_list, int _pid);
pcb_t *ProcListReadyNext(proc_list_t *_proc_list);
int    ProcListReadyPrint(proc_list_t *_proc_list);
//int    ProcListReadyRemove(proc_list_t *_proc_list, int _pid);
//
//

int    ProcListRunningSet(proc_list_t *_proc_list, pcb_t *_process);
pcb_t *ProcListRunningGet(proc_list_t *_proc_list);

//int    ProcListRunningRemove(proc_list_t *_proc_list);
//pcb_t *ProcListRunningSwitch(proc_list_t *_proc_list, pcb_t *_process);
//
//int    ProcListTerminatedAdd(proc_list_t *_proc_list, pcb_t *_process);
//pcb_t *ProcListTerminatedGet(proc_list_t *_proc_list, int _pid);
//int    ProcListTerminatedPrint(proc_list_t *_proc_list);
//int    ProcListTerminatedRemove(proc_list_t *_proc_list, int _pid);
#endif // __PROC_LIST_H