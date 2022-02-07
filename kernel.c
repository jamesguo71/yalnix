#include "kernel.h"


// TODO: Consider making these static inline functions. The inline keyword tells the compiler to
//       replace every function call instance with the code itself where it is being called. This
//       is more efficient in terms of runtime overhead because you do not need to make the function
//       call, but makes the overall code size larger since the code is copied everywhere the function
//       is called. For our case, the code bloat should be really small though.
//Bit Manipulation: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/Progs/bit-array2.c
#define SetBit(A,k)     ( A[(k/sizeof(int))] |= (1 << (k%sizeof(int))) )
#define ClearBit(A,k)   ( A[(k/sizeof(int))] &= ~(1 << (k%sizeof(int))) )
#define TestBit(A,k)    ( A[(k/sizeof(int))] & (1 << (k%sizeof(int)) ) )


/*
 * Local Variable Definitions
 */
static int  g_nframes = 0;     // Number of physical frames available, initialized in KernelStart
static int *g_bit_vec = NULL;  // Bit vector for free frames tracking, initialized in KernelStart
static void g_interrupt_table[TRAP_VECTOR_SIZE] = {
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


/*
 * Local Function Definitions
 */
static int find_free_frame(void);
static void set_pte(pte_t *pt, int index, int valid, int prot, int pfn);
static void DoIdle(void);


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
            if (frameNum > g_bit_vec_len) {
                return ERROR;
            }
            g_bit_vec[frameNum] = 1;              // TODO: Use g_bit_vec operation functions

        } else {

            // 7b. Calculate the frame number for the next frame that we will remove from the heap.
            // Make sure that we actually have a frame to remove! If not, return ERROR.
            // Otherwise, mark the frame as free..
            int frameNum = curBrkFrame - i;
            if (frameNum < 0) {
                return ERROR;
            }
            g_bit_vec[frameNum] = 0;              // TODO: Use g_bit_vec operation functions
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


/*!
 * \desc                 A
 * 
 * \param[in] cmd_args   A
 * \param[in] pmem_size  A
 * \param[in] uctxt      A
 */
void KernelStart (char **cmd_args, unsigned int pmem_size, UserContext *uctxt) {
    // 1. Make sure our user context struct is not NULL and that we
    //    have enough physical memory. If not, halt the machine.
    if (uctxt == NULL || pmem_size < PAGESIZE) {
        Halt();
    }

    // 2. Calculate how many frames we have given our physical memory and page sizes.
    //    Then, calculate the length of our frame bit vector, where each bit is used
    //    to represent whether a frame is free or in use.
    g_nframes = pmem_size / PAGESIZE;
    int int_arr_size = g_nframes / sizeof(int);

    // 3. Allocate space for our frame bit vector. Use calloc so that every
    //    bit starts out as zero (i.e., every frame is currently free).
    g_bit_vec = calloc(int_arr_size); // this is using Kernel Malloc
    if (g_bit_vec == NULL) {
        TracePrintf(1, "Calloc for g_bit_vec failed!\n");
    }

    // 4. The kernel text, data, and heap are loaded into memory starting from PMEM_BASE.
    //    Use the kernel brk (i.e., end of the heap) to determine how many frames are
    //    used to store the kernel text, data, and heap. Then, mark these frames as taken.
    //
    //    TODO: What if we remove this first loop and instead call SetBit in steps 6, 7, 12,
    //          and 15 as we are setting up our page table entries? This loop accomplishes
    //          the same thing, but maybe it makes more sense (in terms of readability) to
    //          mark frames and pages valid at the same time?
    int nk_frames = (_kernel_data_end - PMEM_BASE) / PAGESIZE;
    for (int i = 0; i < nk_frames; i++) {
        SetBit(g_bit_vec, i);
    }

    // 5. Set up the Region 0 page table (i.e., the kernel's page table). Use the size of the
    //    *virtual* memory region 0 to calculate how many entries we will have in the page table.
    //
    //    TODO: Leaving this as a reminder that we may find it useful to make the kernel page
    //          table size a global variable in case we need to use it in other functions.
    //          Also, the kernel_pt is currently defined in kernel.h as "extern" so that other
    //          files may use it; if no other files need it, move it into kernel.c as a local
    //          global variable instead.
    //
    //    TODO: Don't we want to assert that VMEM_0_BASE == VMEM_BASE?
    //
    //    TODO: I just noticed that there is a MAX_PT_LEN; I think we should use this instead
    //          of VMEM_0_SIZE / PAGESIZE (though, technically, they should be the same)
    assert(VMEM_0_BASE == PMEM_BASE);
    int kernel_pt_size = VMEM_0_SIZE / PAGESIZE;
    pte_t *kernel_pt = calloc(kernel_pt_size * sizeof(pte_t));
    if (kernel_pt == NULL) {
        TracePrintf(1, "Calloc for kernel_pt failed!\n");
    }

    // 6. Calculate the number of pages being used to store the kernel text then set their
    //    entries in the page table as valid with proper permissions (text should be both
    //    readable and executable as it contains code).
    //
    //    TODO: _kernel_data_start contains a *physical* address so shouldn't we subtract
    //          PMEM_BASE instead? I know they are both 0 in hardware.h, but theoretically
    //          they could be different.
    int n_kernel_text_pages = (_kernel_data_start - VMEM_0_BASE) / PAGESIZE;
    for(int i = 0; i < n_kernel_text_pages; i++) {
        set_pte(kernel_pt,                  // page table pointer
                i,                          // page number
                1,                          // valid bit
                PROT_READ | PROT_EXEC,      // page protection bits
                i);                         // frame number
    }

    // 7. Calculate the number of pages being used to store the kernel data then set their
    //    entries in the page table as valid with proper permissions (data should be both
    //    readable and writable but not executable).
    //
    //    Assumption: kernel text and kernel data are contiguous ( No Page Gap between them )
    //
    //    TODO: Hey Fei, I removed the free_pfn variable because the logic for this second
    //          for loop was wrong---free_pfn would have equaled n_kernel_text_pages at the
    //          beginning of the loop, so you would have only done
    //          n_kernel_data_pages - n_kernel_text_pages iterations. Instead, I just add
    //          n_kernel_text_pages to i so that we are still indexing into the correct
    //          entry for the page table and frame number and doing correct # of iterations.
    int n_kernel_data_pages = (_kernel_data_end - _kernel_data_start) / PAGESIZE;
    for (int i = 0; i < n_kernel_data_pages; i++) {
        set_pte(kernel_pt,                  // page table pointer
                i + n_kernel_text_pages,    // page number
                1,                          // valid bit
                PROT_READ | PROT_WRITE,     // page protection bits
                i + n_kernel_text_pages);   // frame number
    }

    // 8. Tell the CPU where to find our kernel's page table and how large it is
    WriteRegister(REG_PTBR0, kernel_pt);
    WriteRegister(REG_PTLR0, kernel_pt_size);

    // 9. Set up the Region 1 page table (i.e., the dummy idle process). Use the size of the
    //    *virtual* memory region 1 to calculate how many entries we will have in the page table.
    //
    //    TODO: Again, can't we just use MAX_PT_LEN?
    int user_pt_size = VMEM_1_SIZE / PAGESIZE;
    pte_t *user_pt = calloc(user_pt_size * sizeof(pte_t));
    if (user_pt == NULL) {
        TracePrintf(1, "Calloc for user_pt failed!\n");
    }

    // 10. Find a free frame for the dummy idle process' stack.
    //
    // TODO: IS IT OKAY TO USE THE FRAME ABOVE KERNEL STACK FOR THE FIRST PROCESS'S USER STACK Page?
    //
    // TODO: I think you should take the same approach for the user stack that is taken for the
    //       kernel stack in hardware.h. Specifically, the KERNEL_STACK_LIMIT = VMEM_0_LIMIT. So,
    //       by that logic, our USER_STACK_LIMIT = VMEM_1_LIMIT. I think the following line of code
    //       should give you the number of the last frame in memory region 1.
    //
    //           int userstack_frame = DOWN_TO_PAGE(VMEM_1_LIMIT) >> PAGESHIFT
    //
    // TODO: I think you meant to use PAGESHIFT instead of PAGESIZE?
    int userstack_frame = VMEM_1_BASE >> PAGESIZE;
    set_pte(user_pt,                                // page table pointer
            user_pt_size - 1,                       // page number
            1,                                      // valid bit
            PROT_READ | PROT_WRITE,                 // page protection bits
            userstack_frame);                       // frame number
    SetBit(g_bit_vec, userstack_frame);

    // 11. Initialize a pcb struct to track information for our dummy idle process.
    //
    //     TODO: We will have to add this to some global array so that the pcb can be
    //           accessed and used during interrupts and syscalls.
    pcb_t *idlePCB = malloc(sizeof(pcb_t));
    if (idlePCB == NULL) {
        TracePrintf(1, "Malloc for idlePCB failed!\n");
    }
    idlePCB->pt = user_pt;

    // 12. Initialize the kernel stack for the dummy idle process. Every process has a kernel
    //     stack, the max size of which is defined in hardware.h. Use the max kernel stack size
    //     to calculate how many frames are needed and set their entries in the process' page
    //     table.
    //
    //     TODO: Don't we also need to update our frame bitvector to show that these frames
    //           starting at ks_frame_start are now taken?
    int n_ks_frames = KERNEL_STACK_MAXSIZE / PAGESIZE;
    idlePCB->ks_frames = calloc(n_ks_frames * sizeof(pte_t));
    if (idlePCB->ks_frames == NULL) {
        TracePrintf(1, "Calloc for idlePCB->ks_frames failed!\n");
    }

    int ks_frame_start = KERNEL_STACK_BASE >> PAGESHIFT;
    for (int ks_i = 0; ks_i < n_ks_frames; ks_i++) {
        set_pte(idlePCB->ks_frames,         // page table pointer
                ks_i,                       // page number
                1,                          // valid bit
                PROT_READ | PROT_WRITE,     // page protection bits
                ks_i + ks_frame_start);     // frame number
        ks_frame_start++;
    }
    memcpy(&kernel_pt[KERNEL_STACK_BASE >> PAGESHIFT], idlePCB->ksframes, n_ks_frames * sizeof(pte_t));

    // 13. Initialize the user context struct for our dummy idle process. Set the program counter
    //     to point to our DoIdle function, and the stack pointer to the end of region 1 (i.e., to
    //     the top of the stack region we setup in step 12).
    //
    //     TODO: I think we may need to use VMEM_1_LIMIT - 1, or something similar, becase the
    //           guide states that VMEM_1_LIMIT is the "first byte *above* the region".
    idlePCB->uctxt = malloc(sizeof(UserContext));
    if (idlePCB->uctxt == NULL) {
        TracePrintf(1, "Malloc for UserContext failed!\n");
    }
    memcpy(idlePCB->uctxt, uctxt, sizeof(UserContext));
    idlePCB->uctxt->pc = DoIdle;
    idlePCB->uctxt->sp = VMEM_1_LIMIT;
    idlePCB->pid = helper_new_pid();

    // 14. Tell the CPU where to find our dummy idle's page table and how large it is
    //
    //     TODO: Again, shouldn't we use MAX_PT_LEN?
    WriteRegister(REG_PTBR1, user_pt);
    WriteRegister(REG_PTLR1, user_pt_size);
    WriteRegister(REG_VECTOR_BASE, g_interrupt_table);

    // 15. Initialize the page table and frame bit vector entries for the kernel's heap.
    //     Since we have been using frames sequentially for the kernel text and data, then
    //     the start frame for the kernel heap is num_of_text_pages + num_of_data_pages.
    //
    //     TODO: You need to set _kernel_curr_brk = _kernel_orig_brk back at the beginning
    //           of KernelStart *before* you ever call calloc/malloc for this to work!
    int n_kernel_heap_pages = (_kernel_curr_brk - _kernel_data_end) / PAGESIZE;
    int n_kernel_heap_start = n_kernel_text_pages + n_kernel_data_pages;
    for (int i = 0; i < n_kernel_heap_pages; i++) {
        set_pte(kernel_pt,                          // page table pointer
                i + n_kernel_heap_start,            // page number
                1,                                  // valid bit
                PROT_READ | PROT_WRITE,             // page protection bits
                i + n_kernel_heap_start);           // frame number
        SetBit(g_bit_vec, i + n_kernel_heap_start);
    }

    // 16. Enable virtual memory!
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
 * \desc                  A
 *                       
 *                       
 * 
 * \param[in] kc_in       a pointer to a temporary copy of the current kernel context of the caller
 * \param[in] curr_pcb_p  a void pointer to current process's pcb
 * \param[in] next_pcb_p  a void pointer to the next process's pcb
 * 
 * \return                return a pointer to a kernel context it had earlier saved in the next
 *                        process’s PCB.
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


/*!
 * \desc    Find a free frame from the physical memory
 * 
 * \return  Frame index on success, ERROR otherwise.
 */
static int find_free_frame(void) {
    for (int i = 0; i < g_nframes; i++) {
        if (TestBit(g_bit_vec, i) == 0) {
            return i;
        }
    }
    return ERROR;
}


/*!
 * \desc             A
 * 
 * \param[in] pt     T
 * \param[in] index  T
 * \param[in] valid  T
 * \param[in] prot   T
 * \param[in] pfn    T
 */
static void set_pte(pte_t *pt, int index, int valid, int prot, int pfn) {
    pt[index].valid = valid;
    pt[index].prot  = prot;
    pt[index].pfn   = pfn;
}


/*!
 * \desc  A dummy userland process that the kernel runs whenever there are no other processes.
 */
static void DoIdle(void) {
    while(1) {
        TracePrintf(1,"DoIdle\n");
        Pause();
    }
}
