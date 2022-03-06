#include <hardware.h>
#include <ykernel.h>
#include "frame.h"
#include "kernel.h"
#include "process.h"
#include "pte.h"
#include "syscall.h"

/*!
 * \desc    Initializes memory for a new pcb_t struct
 *
 * \return  An initialized pcb_t struct, NULL otherwise.
 */
pcb_t *ProcessCreate() {
    // 1. Call ProcessCreateIdle since it completes most of the steps that we would do for
    //    a generic process. Then afterwards, all we have to do is map a kernel stack.
    pcb_t *process = ProcessCreateIdle();
    if (!process) {
        TracePrintf(1, "[ProcessCreate] Failed to create process\n");
        return NULL;
    }

    // 2. Find some free frames for the process' kernel stack and map them to the pages in
    //    the process' kernel stack page table.
    TracePrintf(1, "[ProcessCreate] Mapping kernel stack pages for pid: %d\n", process->pid);
    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        int frame = FrameFindAndSet();
        if (frame == ERROR) {
            ProcessDestroy(process);
            TracePrintf(1, "[ProcessCreate]: Failed to find a free frame.\n");
            return NULL;
        }
        PTESet(process->ks,                         // page table pointer
               i,                                   // page number
               PROT_READ | PROT_WRITE,              // page protection bits
               frame);                              // frame number
        TracePrintf(1, "[ProcessCreate] Mapping page: %d to frame: %d\n", i, frame);
    }
    // 3. Create a linked list for storing the resources (pipe, lock, cvar) the process created
    process->res_list = list_new();

    return process;
}

pcb_t *ProcessCreateIdle() {
    // 1. Allocate space for our process struct. Print message and return NULL upon error
    pcb_t *process = (pcb_t *) malloc(sizeof(pcb_t));
    if (!process) {
        TracePrintf(1, "[ProcessCreateIdle] Error mallocing space for process struct\n");
        return NULL;
    }

    bzero(process->ks, sizeof(process->ks[0]) * KERNEL_NUMBER_STACK_FRAMES);
    bzero(process->pt, sizeof(process->pt[0]) * MAX_PT_LEN);

    // 2. Set the KernelContext to NULL as it will be initialized later by KCCopy. Similarly,
    //    Set the brk and data_end addresses to NULL as they will be set later by LoadProgram.
    process->kctxt    = NULL;
    process->brk      = NULL;
    process->data_end = NULL;
    process->parent = NULL;
    process->headchild = NULL;
    process->sibling = NULL;
    process->res_list = NULL;


    // 3. Assign the process a pid. Note that the build system keeps a mappig of page tables
    //    to pids, so if we don't assign pid via the helper function it complains about the
    //    PTBR1 not being assigned to a process.
    process->pid = helper_new_pid(process->pt);
    return process;
}


/*!
 * \desc                  Destroy the process by ProcessTerminate it and ProcessDelete it
 *
 * \param[in] _pcb  A pcb_t struct that the caller wishes to destroy
 */
void ProcessDestroy(pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_process) {
        TracePrintf(1, "[ProcessDestroy] Invalid pcb pointer\n");
        Halt();
    }
    ProcessTerminate(_process);
    ProcessDelete(_process);
}

/*!
 * \desc                  remove the relationships for this pcb and retire its pid before freeing its memory
 *
 * \param[in] _pcb  A pcb_t struct that the caller wishes to free
 */
 void ProcessDelete(pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_process) {
        TracePrintf(1, "[ProcessTerminate] Invalid pcb pointer\n");
        Halt();
    }

    if (_process->kctxt) {
        free(_process->kctxt);
    }

    // remove itself from its parent's children list
    if (_process->parent) {
        ProcessRemoveChild(_process->parent, _process);
    }

    // this will change its children's parent pointers to null
    for (pcb_t *end = _process->headchild; end != NULL; end = end->sibling) {
        end->parent = NULL;
    }

    helper_retire_pid(_process->pid);

    list_foreach(_process->res_list, SyscallReclaim);
    list_free(_process->res_list);

    free(_process);
 }

/*!
 * \desc                this will free the resources, frames, pagetable entries of the pcb
 *
 * \param[in] _process  The pcb struct for the process the caller wishes to terminate
 */
void ProcessTerminate(pcb_t *_process) {
    // 1. Check arguments. Return error if invalid.
    if (!_process) {
        TracePrintf(1, "[ProcessTerminate] Invalid pcb pointer\n");
        Halt();
    }

    // 2. Free the current process' memory by freeing its frames (both region 1 and region 0).
    for (int i = 0; i < MAX_PT_LEN; i++) {
        if (_process->pt[i].valid) {
            FrameClear(_process->pt[i].pfn);
            PTEClear(_process->pt, i);
        }
    }

    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        if (_process->ks[i].valid) {
            FrameClear(_process->ks[i].pfn);
            PTEClear(_process->ks, i);
        }
    }
}

/*!
 * \desc    Add a child process's link to its parent
 */
void ProcessAddChild(pcb_t *_parent, pcb_t *_child) {
    if (!_parent || !_child) {
        TracePrintf(1, "[ProcessAddChild] invalid pointer.\n");
        Halt();
    }

    if (_parent->headchild == NULL) {
        _parent->headchild = _child;
    }
    else {
        pcb_t *end;
        for (end = _parent->headchild; end->sibling != NULL; end = end->sibling)
            ;
        end->sibling = _child;
    }
}

/*!
 * \desc    Remove a child process's link from its parent
 */
void ProcessRemoveChild(pcb_t *_parent, pcb_t *_child) {
    if (!_parent || !_child) {
        TracePrintf(1, "[ProcessAddChild] invalid pointer.\n");
        Halt();
    }

    if (_parent->headchild == _child) {
        _parent->headchild = _child->sibling;
    }
    else {
        pcb_t *end;
        for (end = _parent->headchild; end->sibling != _child; end = end->sibling)
            ;
        end->sibling = _child->sibling;
    }
}