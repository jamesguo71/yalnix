#include "kernel.h"


/*!
 * \desc            Sets the kernel's brk (i.e., the address right above the kernel heap)
 * 
 * \param[in] _brk  The address of the new brk
 * 
 * \return          0 on success, ERROR otherwise.
 */
int SetKernelBrk(void *_brk) {
    // 1. Check arguments. Return error if invalid. The new brk should be
    //    (1) a valid pointer and (2) should not point below the heap.
    if (!_brk) {
        return ERROR;
    }

    if (_brk <= _kernel_data_end) {
        return ERROR;
    }

    // 2. Round our new brk value *up* to the nearest page
    UP_TO_PAGE(_brk);

    // 3. Check to see if we are growing or shrinking the brk and calculate the difference
    int growing    = 0;
    int difference = 0;
    if (_brk > _kernel_curr_brk) {
        growing    = 1;
        difference = _brk - _kernel_curr_brk;
    } else {
        difference = _kernel_curr_brk - _brk;
    }

    // 4. Calculate the number of frames we need to add or remove.
    int numFrames = difference / PAGE_SIZE;

    // 5. Add or remove frames/pages based on whether we are growing or shrinking.
    //    I imagine we will have some list to track the available frames and a 
    //    list for the kernels pages, along with some getter/setter helper
    //    functions for modifying each.
    for (int i = 0; i < numFrames; i++) {

        if (growing) {

            // Find a free frame. If NULL is returned, we must be out of memory!
            frame = getFreeFrame();
            if (!frame) {
                return ERROR;
            }

            // Update the kernel's page table. Somehow I am supposed to account for
            // whether we have initialized virtual memory or not.
            kernelAddPage(frame);
        } else {

            // Remove the last page. Would this ever return error?
            frame = kernelRemovePage();
            if (!frame) {
                return ERROR;
            }

            // Mark that frame as free
            addFreeFrame(frame);
        }
    }

    // 6. Set the kernel brk to the new brk value and return a success code
    _kernel_curr_brk = _brk 
    return 0;
}

void KernelStart (char **cmd_args, unsigned int pmem_size, UserContext *uctxt) {
    // 1. Check arguments. Return error if invalid.
    // Check if cmd_args are blank. If blank, kernel starts to look for a executable called “init”. 
    // Otherwise, load `cmd_args[0]` as its initial process.

    // For Checkpoint 2, create an idlePCB that keeps track of the idld process.
    // Write a simple idle function in the kernel text, and
    // In the UserContext in idlePCB, set the pc to point to this code and the sp to point towards the top of the user stack you set up.
    // Make sure that when you return to user mode at the end of KernelStart, you return to this modified UserContext.

    // For Checkpoint 3, KernelStart needs to create an initPCB and Set up the identity for the new initPCB
    // Then write KCCopy() to: copy the current KernelContext into initPCB and
    // copy the contents of the current kernel stack into the new kernel stack frames in initPCB

    // Should traceprint “leaving KernelStart” at the end of KernelStart.
}

KernelContext *MyKCS(KernelContext *, void *, void *) {
    // Copy the kernel context into the current process’s PCB and return a pointer to a kernel context it had earlier saved in the next process’s PCB.
}


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
KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used) {
    // 1. Check arguments. Return error if invalid.
}
