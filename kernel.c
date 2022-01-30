#include "kernel.h"


int SetKernelBrk(void *_brk) {
    // 1. Check arguments. Return error if invalid. The new brk should be (1) a valid
    //    pointer that is (2) not below our heap and (3) not in our stack space.
    if (!_brk) {
        return ERROR_CODE;
    }

    if (_brk < heapBottom || _brk > stackBottom) {
        return ERROR_CODE;
    }

    // 2. Check to see if we are growing or shrinking the brk and calculate the difference
    int growing    = 0;
    int difference = 0;
    if (_brk > brk) {
        growing    = 1;
        difference = _brk - brk;
    } else {
        difference = brk - _brk;
    }

    // 3. Calculate the number of frames we need to add or remove.
    //
    // TODO: We need to consider what happens when difference is < FRAME_SIZE.
    //       It may be the case that brk is only increased or decreased by a
    //       small amount. If so, it may be the case that we do not need to
    //       add or remove any pages.
    int numFrames = difference / FRAME_SIZE;
    if (difference % FRAME_SIZE) {
        numFrames++;
    }

    // 4. Add or remove frames/pages based on whether we are growing or shrinking.
    //    I imagine we will have some list to track the available frames and a 
    //    list for the kernels pages, along with some getter/setter helper
    //    functions for modifying each.
    for (int i = 0; i < numFrames; i++) {

        // TODO: I think we need to consider what the virtual address for
        //       the new page will be. Specifically, it should be the
        //       current last kernel page addr + FRAME_SIZE.
        if (growing) {
            frame = getFreeFrame();
            kernelAddPage(frame, pageAddress);
        } else {
            frame = kernelRemovePage(pageAddress);
            addFreeFrame(frame);
        }
    }

    // 5. Set the kernel brk to the new brk value and return a success code
    brk = _brk 
    return SUCCESS_CODE;
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

KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used) {
    // 1. Check arguments. Return error if invalid.
}
