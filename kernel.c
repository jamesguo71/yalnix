#include "kernel.h"
#include "../include/hardware.h"

/*!
 * \desc            Sets the kernel's brk (i.e., the address right above the kernel heap)
 * 
 * \param[in] _brk  The address of the new brk
 * 
 * \return          0 on success, ERROR otherwise.
 */
int SetKernelBrk(void *_brk) {
    // 1. Make sure the incoming address is not NULL. If so, return ERROR.
    if (!_brk) {
        return ERROR;
    }

    // 2. Round our new brk value *up* to the nearest page
    UP_TO_PAGE(_brk);

    // 3. Check to see it we have enabled virtual memory. If it is, use our internal
    //    helper function to handle the brk under our virtual memory model.
    if (g_virtual_memory) {
        return SetKernelBrkVirtual(_brk);
    }

    // 4. If virtual memory is not enabled, then the process for changing the brk is simpler
    //    and is as follows:
    //        (1) Make sure the new brk does not point below our heap. If so, return ERROR
    //        (2) Determine whether we are growing or shrinking the heap
    //        (3) Find or free frames depending on whether we are growing or shrinking
    //        (4) Update our current brk pointer
    //
    //    NOTE: Currently, we may not maintain the exact brk value provided by the caller as it
    //          rounds the brk up to the nearest page boundary. In other words, the only valid
    //          brk values are those that fall on page boundaries, so the kernel may end up with
    //          *more* memory than it asked for. It should never end up freeing more memory than
    //          it specified, however, because we round up.
    if (_brk <= _kernel_data_end) {
        return ERROR;
    }

    // 5. Check to see if we are growing or shrinking the brk and calculate the difference
    int growing    = 0;
    int difference = 0;
    if (_brk > _kernel_curr_brk) {
        growing    = 1;
        difference = _brk - _kernel_curr_brk;
    } else {
        difference = _kernel_curr_brk - _brk;
    }

    // 6. Calculate the number of frames we need to add or remove. Furthermore, calculate the
    //    frame number for the current brk, as we will use it to index into our frame bit
    //    vector. Specifically, we want to add/remove frames starting with the frame that
    //    our current brk points to.
    int numFrames   = difference       / PAGE_SIZE;
    int curBrkFrame = _kernel_curr_brk / PAGE_SIZE;

    // 7. Add or remove frames/pages based on whether we are growing or shrinking.
    for (int i = 0; i < numFrames; i++) {

        if (growing) {

            // 7a. Calculate the frame number for the next frame that we will add to the heap.
            // Make sure that we actually have a frame to add! If not, return ERROR.
            // Otherwise, mark the frame as taken.
            int frameNum = curBrkFrame + i;
            if (frameNum > bit_vec_len) {
                return ERROR;
            }
            bit_vec[frameNum] = 1;              // TODO: Use bit_vec operation functions

        } else {

            // 7b. Calculate the frame number for the next frame that we will remove from the heap.
            // Make sure that we actually have a frame to remove! If not, return ERROR.
            // Otherwise, mark the frame as free..
            int frameNum = curBrkFrame - i;
            if (frameNum < 0) {
                return ERROR;
            }
            bit_vec[frameNum] = 0;              // TODO: Use bit_vec operation functions
        }
    }

    // 8. Set the kernel brk to the new brk value and return success
    //    TODO: I think that KernelStart needs to initialize _kernel_curr_brk
    //    *before* it makes any calls to malloc (or enables virtual memory).
    //    Specifically, it should:
    //
    //        _kernel_curr_brk = _kernel_orig_brk;
    _kernel_curr_brk = _brk 
    return 0;
}

int nframes = 0; // Number of physical frames available
int *bit_vec; // Bit vector for free frames tracking
//Bit Manipulation: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/Progs/bit-array2.c
#define SetBit(A,k)     ( A[(k/32)] |= (1 << (k%32)) )
#define ClearBit(A,k)   ( A[(k/32)] &= ~(1 << (k%32)) )
#define TestBit(A,k)    ( A[(k/32)] & (1 << (k%32)) )

void interrupt_table[TRAP_VECTOR_SIZE] = {
        trap_kernel,
        trap_clock,
        trap_illegal,
        trap_memory,
        trap_math,
        trap_tty_receive,
        trap_tty_transmit,
        trap_disk,
        // ToDo: Eight pointers pointing to "this trap is not yet handled" handler
        NULL,NULL,NULL,NULL,
        NULL,NULL,NULL,NULL
}
void KernelStart (char **cmd_args, unsigned int pmem_size, UserContext *uctxt) {
    // 1. Check arguments. Return error if invalid.
    // Check if cmd_args are blank. If blank, kernel starts to look for a executable called “init”. 
    // Otherwise, load `cmd_args[0]` as its initial process.

    nframes = pmem_size / PAGESIZE;
    int int_arr_size = nframes/sizeof(int);
    bit_vec = malloc(int_arr_size); // this is using Kernel Malloc
    for (int i = 0; i < int_arr_size; i++)
        // To begin, all frames are unused
        bit_vec[i] = 0;
    // ASSUMPTION: PMEM_BASE to _kernel_orig_brk are used physical memeroy
    // ASSUMPTION: _kernel_orig_brk is a multiple of PAGESIZE
    int kernel_used_frames = (_kernel_orig_brk - PMEM_BASE) / PAGESIZE;
    for (int i = 0; i < kernel_used_frames; i++) {
        SetBit(bit_vec, i);
    }
    int pt_size = VMEM_SIZE / PAGESIZE;
    pte_t *pt = malloc(pt_size * sizeof(pte_t));


    WriteRegister(REG_VECTOR_BASE, interrupt_table);






    // For Checkpoint 2, create an idlePCB that keeps track of the idld process.
    // Write a simple idle function in the kernel text, and
    // In the UserContext in idlePCB, set the pc to point to this code and the sp to point towards the top of the user stack you set up.
    // Make sure that when you return to user mode at the end of KernelStart, you return to this modified UserContext.

    // For Checkpoint 3, KernelStart needs to create an initPCB and Set up the identity for the new initPCB
    // Then write KCCopy() to: copy the current KernelContext into initPCB and
    // copy the contents of the current kernel stack into the new kernel stack frames in initPCB

    // Should traceprint “leaving KernelStart” at the end of KernelStart.
}

/*!
 * \desc                 
 *                       
 *                       
 * 
 * \param[in] kc_in         a pointer to a temporary copy of the current kernel context of the caller
 * \param[in] curr_pcb_p    a void pointer to current process's pcb
 * \param[in] next_pcb_p    a void pointer to the next process's pcb
 * 
 * \return                  return a pointer to a kernel context it had earlier saved in the next process’s PCB.
 */
KernelContext *MyKCS(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    // Check if parameters are null
    // Copy the kernel context into the current process’s PCB
    // Change Page Table Register  REG_PTBR0 for the new Region 1 address of the new process
    // Change kernel stack page table entries for next process
    // Flush TLB for kernel stack and Region 1
    // return a pointer to a kernel context it had earlier saved in the next process’s PCB.
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
    if (!kc_in || !new_pcb_p) {
        return NULL;
    }

    // 2. Cast our new process pcb pointer so that we can reference the internal variables.
    pcb_t *pcb = (pcb_t *) new_pcb_p;

    // 3. Copy the incoming KernelContext into the new process' pcb. Can I use memcpy
    //    in the kernel or do I need to implement it myself (i.e., use void pointers
    //    and loop over copying byte-by-byte)? I bet I have to do it manually...
    memcpy(pcb->kctxt, kc_in, sizeof(KernelContext));

    // 4. I'm supposed to copy the current kernel stack frames over to the new process' pcb, but
    //    I do not understand how I access the current kernel stack frames with only kc_in. Is
    //    it something related to kstack_cs? What is that variable?
    for (int i = 0; i < numFrames; i++) {
        // copy frames over somehow???
    }

    // x. Return the incoming KernelContext
    return kc_in
}
