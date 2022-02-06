#include "kernel.h"
#include "../include/yalnix.h"
#include "../include/hardware.h"
#include "trap.h"

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


int nframes = 0; // Number of physical frames available, initialized in KernelStart
int *bit_vec; // Bit vector for free frames tracking, initialized in KernelStart
//Bit Manipulation: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/Progs/bit-array2.c

#define SetBit(A,k)     ( A[(k/sizeof(int))] |= (1 << (k%sizeof(int))) )
#define ClearBit(A,k)   ( A[(k/sizeof(int))] &= ~(1 << (k%sizeof(int))) )
#define TestBit(A,k)    ( A[(k/sizeof(int))] & (1 << (k%sizeof(int)) ) )
/*
 * Find a free frame from the physical memory
 * if succeeded, return the index
 * Otherwise, return ERROR
 */
int find_free_frame(void) {
    for (int i = 0; i < nframes; i++) {
        if (TestBit(bit_vec, i) == 0) {
            return i;
        }
    }
    return ERROR;
}

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
        trap_not_handled,
        trap_not_handled,,
        trap_not_handled,
        trap_not_handled,
        trap_not_handled,
        trap_not_handled,
        trap_not_handled,
        trap_not_handled,
}

void set_pte(pte_t *pt, int index, int valid, int prot, int pfn) {
    pt[index].valid = valid;
    pt[index].prot = prot;
    pt[index].pfn = pfn;
}

void DoIdle(void) {
    while(1) {
        TracePrintf(1,"DoIdle\n");
        Pause();
    }
}

void KernelStart (char **cmd_args, unsigned int pmem_size, UserContext *uctxt) {
    if (uctxt == NULL || pmem_size < PAGESIZE) {
        Halt();
    }

    // Set up a way to track free frames
    nframes = pmem_size / PAGESIZE;
    int int_arr_size = nframes/sizeof(int);
    // To begin, all frames are unused
    bit_vec = calloc(int_arr_size); // this is using Kernel Malloc
    if (bit_vec == NULL) {
        TracePrintf(1, "Calloc for bit_vec failed!\n");
    }

    // number of frames used by Kernel Text and Kernel Data
    int nk_frames = (_kernel_data_end - PMEM_BASE) / PAGESIZE;
    for (int i = 0; i < nk_frames; i++) {
        SetBit(bit_vec, i);
    }

    // Set up the initial Region 0 page table
    assert(VMEM_0_BASE == PMEM_BASE);
    int kernel_pt_size = VMEM_0_SIZE / PAGESIZE;
    pte_t *kernel_pt = calloc(kernel_pt_size * sizeof(pte_t));
    if (kernel_pt == NULL) {
        TracePrintf(1, "Calloc for kernel_pt failed!\n");
    }

    // this variable will be used for indexing into kernel page table entries
    int free_pfn = 0;

    int n_kernel_text_pages = (_kernel_data_start - VMEM_0_BASE) / PAGESIZE;
    for(; free_pfn < n_kernel_text_pages; free_pfn++) {
        set_pte(kernel_pt, free_pfn, 1, PROT_READ | PROT_EXEC, free_pfn);
    }

    // Assumption: kernel text and kernel data are contiguous ( No Page Gap between them )
    int n_kernel_data_pages = (_kernel_data_end - _kernel_data_start) / PAGESIZE;
    for (; free_pfn < n_kernel_data_pages; free_pfn++) {
        set_pte(kernel_pt, free_pfn, 1, PROT_READ | PROT_WRITE, free_pfn);
    }

    WriteRegister(REG_PTBR0, kernel_pt);
    WriteRegister(REG_PTLR0, kernel_pt_size);

    // Set up a Region 1 page table for idle.
    int user_pt_size = VMEM_1_SIZE / PAGESIZE;
    pte_t *user_pt = calloc(user_pt_size * sizeof(pte_t));
    if (user_pt == NULL) {
        TracePrintf(1, "Calloc for user_pt failed!\n");
    }
    // This should have one valid page, for idle’s user stack.
    // TODO: IS IT OKAY TO USE THE FRAME ABOVE KERNEL STACK FOR THE FIRST PROCESS'S USER STACK Page?
    int userstack_frame = VMEM_1_BASE >> PAGESIZE;
    set_pte(user_pt, user_pt_size - 1, 1, PROT_READ | PROT_WRITE, userstack_frame);
    SetBit(bit_vec, userstack_frame);

    // Make that formal by creating an idlePCB that keeps track of this identity:
    pcb_t *idlePCB = malloc(sizeof(pcb_t));
    if (idlePCB == NULL) {
        TracePrintf(1, "Malloc for idlePCB failed!\n");
    }
    idlePCB->pt = user_pt;
    // Set up its kernel stack frames
    int n_ks_frames = KERNEL_STACK_MAXSIZE / PAGESIZE;
    idlePCB->ks_frames = calloc(n_ks_frames * sizeof(pte_t));
    if (idlePCB->ks_frames == NULL) {
        TracePrintf(1, "Calloc for idlePCB->ks_frames failed!\n");
    }
    int ks_frame_start = KERNEL_STACK_BASE >> PAGESHIFT;
    for (int ks_i = 0; ks_i < n_ks_frames; ks_i++) {
        set_pte(idlePCB->ks_frames, ks_i, 1, PROT_READ|PROT_WRITE, ks_frame_start);
        ks_frame_start++;
    }
    memcpy(&kernel_pt[KERNEL_STACK_BASE >> PAGESHIFT], idlePCB->ksframes, n_ks_frames * sizeof(pte_t));

    // In the UserContext in idlePCB, set the pc to point to this code and the sp to point towards the top of the user stack you set up.
    idlePCB->uctxt = malloc(sizeof(UserContext));
    if (idlePCB->uctxt == NULL) {
        TracePrintf(1, "Malloc for UserContext failed!\n");
    }
    memcpy(idlePCB->uctxt, uctxt, sizeof(UserContext));
    idlePCB->uctxt->pc = DoIdle;
    idlePCB->uctxt->sp = VMEM_1_LIMIT;
    idlePCB->pid = helper_new_pid();

    WriteRegister(REG_PTBR1, user_pt);
    WriteRegister(REG_PTLR1, user_pt_size);

    WriteRegister(REG_VECTOR_BASE, interrupt_table);

    // Enable virtual memory.
    // Note that if the kernel brk has been raised since you built your Region 0 page table,
    // you need to adjust the page table appropriately before turning VM on
    int n_kh_frames = (_kernel_curr_brk - _kernel_orig_brk) >> PAGESHIFT;
    for ( ; free_pfn < (_kernel_data_end >> PAGESHIFT) + n_kh_frames; free_pfn++) {
        set_pte(kernel_pt, free_pfn, 1, PROT_READ|PROT_WRITE, free_pfn);
        SetBit(bit_vec, free_pfn);
    }

    WriteRegister(REG_VM_ENABLE, 1);

    // Make sure that when you return to user mode at the end of KernelStart, you return to this modified UserContext.
    // ASSUMPTION: we can return to the modified UserContext by overriding the given uctxt
    memset(uctxt, idlePCB->uctxt, sizeof(UserContext));

    // Check if cmd_args are blank. If blank, kernel starts to look for a executable called “init”.
    // Otherwise, load `cmd_args[0]` as its initial process.

    // For Checkpoint 3, KernelStart needs to create an initPCB and Set up the identity for the new initPCB
    // Then write KCCopy() to: copy the current KernelContext into initPCB and
    // copy the contents of the current kernel stack into the new kernel stack frames in initPCB

    // Should traceprint “leaving KernelStart” at the end of KernelStart.
    TracePrintf(1, "leaving KernelStart\n");
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
