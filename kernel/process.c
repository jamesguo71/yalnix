#include <hardware.h>
#include <ykernel.h>
#include "frame.h"
#include "kernel.h"
#include "process.h"
#include "pte.h"


/*!
 * \desc    Initializes memory for a new pcb_t struct
 *
 * \return  An initialized pcb_t struct, NULL otherwise.
 */
pcb_t *ProcessCreate() {
    // 1. Allocate space for our process struct. Print message and return NULL upon error
    pcb_t *process = (pcb_t *) malloc(sizeof(pcb_t));
    if (!process) {
        TracePrintf(1, "[ProcessCreate] Error mallocing space for process struct\n");
        Halt();
    }

    // 2. Allocate space for the Region 1 page table
    process->pt = (pte_t *) calloc(MAX_PT_LEN, sizeof(pte_t));
    if (!process->pt) {
        TracePrintf(1, "[ProcessCreate] Calloc for process->pt failed!\n");
        Halt();
    }

    // 3. Allocate space for the Region 0 kernel stack table. Note that we only need
    //    enough space to hold pages for the stack (not for the entire address space).
    process->ks = (pte_t *) calloc(KERNEL_NUMBER_STACK_FRAMES, sizeof(pte_t));
    if (!process->ks) {
        TracePrintf(1, "[ProcessCreate] Calloc for process kernel stack failed!\n");
        Halt();
    }

    // 4. Set the KernelContext to NULL. Later on when the process first gets run, our context
    //    switching code will see that KC is NULL and call KCCopy to copy the KernelContext of
    //    the current running process.
    process->kctxt = NULL;

    // 5. Allocate space for the UserContext.
    process->uctxt = (UserContext *) malloc(sizeof(UserContext));
    if (!process->uctxt) {
        TracePrintf(1, "[ProcessCreate] Malloc for process uctxt failed\n");
        Halt();
    }

    // 6. Set the KernelContext to NULL as it will be initialized later by KCCopy. Similarly,
    //    Set the brk and data_end addresses to NULL as they will be set later by LoadProgram.
    process->kctxt    = NULL;
    process->brk      = NULL;
    process->data_end = NULL;

    // 7. Assign the process a pid. Note that the build system keeps a mappig of page tables
    //    to pids, so if we don't assign pid via the helper function it complains about the
    //    PTBR1 not being assigned to a process.
    process->pid = helper_new_pid(process->pt);

    // 8. Find some free frames for the process' kernel stack and map them to the pages in
    //    the process' kernel stack page table.
    TracePrintf(1, "[ProcessCreate] Mapping kernel stack pages for pid: %d\n", process->pid);
    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        int frame = FrameFind();
        PTESet(process->ks,                         // page table pointer
               i,                                   // page number
               PROT_READ | PROT_WRITE,              // page protection bits
               frame);                              // frame number
        TracePrintf(1, "[ProcessCreate] Mapping page: %d to frame: %d\n", i, frame);
    }
    return process;
}


/*!
 * \desc                  Frees the memory associated with a pcb_t struct
 *
 * \param[in] _pcb  A pcb_t struct that the caller wishes to free
 */
int ProcessDelete(pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_process) {
        TracePrintf(1, "[ProcessDelete] Invalid pcb pointer\n");
        return ERROR;
    }

    // 2. TODO: Loop over every list and free nodes. Then free pcb struct
    if (_process->kctxt) {
        free(_process->kctxt);
    }
    if(_process->uctxt) {
        free(_process->uctxt);
    }
    free(_process);
    return 0;
}