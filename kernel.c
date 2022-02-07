#include "kernel.h"


// TODO: Consider making these static inline functions. The inline keyword tells the compiler to
//       replace every function call instance with the code itself where it is being called. This
//       is more efficient in terms of runtime overhead because you do not need to make the function
//       call, but makes the overall code size larger since the code is copied everywhere the function
//       is called. For our case, the code bloat should be really small though.
//Bit Manipulation: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/Progs/bit-array2.c
#define SetFrame(A,k)     ( A[(k/8)] |= (1 << (k%8)) )
#define ClearFrame(A,k)   ( A[(k/8)] &= ~(1 << (k%8)) )
#define TestFrame(A,k)    ( A[(k/8)] & (1 << (k%8) ) )


/*
 * Local Variable Definitions
 */
static char *g_frames         = NULL;   // Bit vector to track frames (set in KernelStart)
static int   g_num_frames     = 0;      // Number of frames           (set in KernelStart)
static int   g_virtual_memory = 0;      // Flag for tracking whether virtual memory is enabled
static void  g_interrupt_table[TRAP_VECTOR_SIZE] = {
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
static void DoIdle(void);
static int  FindFreeFrame(void);
static void SetPTE(pte_t *pt, int index, int valid, int prot, int pfn);


/*!
 * \desc            Sets the kernel's brk (i.e., the address right above the kernel heap)
 * 
 * \param[in] _brk  The address of the new brk
 * 
 * \return          0 on success, ERROR otherwise.
 */
int SetKernelBrk(void *_kernel_new_brk) {
    // 1. Make sure the incoming address is not NULL and that it does
    //    not point below our heap boundary. If so, return ERROR.
    if (!_kernel_new_brk) {
        TracePrintf(1, "[SetKernelBrk] Error: proposed brk is NULL\n");
        return ERROR;
    }

    if (_kernel_new_brk <= _kernel_data_end) {
        TracePrintf(1, "[SetKernelBrk] Error: proposed brk is below heap base\n");
        return ERROR;
    }

    // 2. Round our new brk value *up* to the nearest page.
    // 
    //    NOTE: Currently, we do not maintain the exact brk value provided by the caller as it
    //          rounds the brk up to the nearest page boundary. In other words, the only valid
    //          brk values are those that fall on page boundaries, so the kernel may end up with
    //          *more* memory than it asked for. It should never end up freeing more memory than
    //          it specified, however, because we round up.
    //
    //    TODO: Do I need to round up? I need to think about this more...
    _kernel_new_brk = UP_TO_PAGE(_kernel_new_brk);

    // 3. If virtual memory has not yet been enabled, then simply save the proposed brk.
    if (!g_virtual_memory) {
        e_kernel_curr_brk = _kernel_new_brk;
        return 0;
    }

    // 4. If virtual memory has been enabled, then we are responsible for updating the kernel's
    //    page table to reflect any pages/frames that have been added/removed as a result of
    //    the brk change. Start off by calculating the page numbers for our new proposed brk
    //    and our current brk.
    int new_brk_page_num = _kernel_new_brk   >> PAGESHIFT;
    int cur_brk_page_num = e_kernel_curr_brk >> PAGESHIFT;

    // 5. Check to see if we are growing or shrinking the brk and calculate the number
    //    of pages that we either need to add or remove given the new proposed brk.
    int growing   = 0;
    int num_pages = 0;
    if (_kernel_new_brk > e_kernel_curr_brk) {
        growing   = 1;
        num_pages = new_brk_page_num - cur_brk_page_num;
    } else {
        num_pages = cur_brk_page_num - new_brk_page_num;
    }

    // 6. Add or remove frames/pages based on whether we are growing or shrinking the heap.
    for (int i = 0; i < num_pages; i++) {

        if (growing) {
            // 6a. If we are growing the heap, then we first need to find an available frame.
            //     If we can't find one (i.e., we are out of memory) return ERROR.
            int frame_num = FindFreeFrame();
            if (frame_num == ERROR) {
                TracePrintf(1, "[SetKernelBrk] Unable to find free frame\n");
                return ERROR;
            }

            // 6b. Map the frame to a page in the kernel's page table. Specifically, start with the
            //     current page pointed to by the kernel brk. Afterwards, mark the frame as in use.
            SetPTE(kernel_pt,               // page table pointer
                   cur_brk_page_num + i,    // page number
                   1,                       // valid bit
                   PROT_READ | PROT_WRITE,  // page protection bits
                   frame_num);              // frame number

            SetFrame(g_frames,              // frame bit vector pointer
                     frame_num);            // frame number

        } else {
            // 6c. If we are shrinking the heap, then we need to unmap pages. Start by grabbing
            //     the number of the frame mapped to the current brk page. Free the frame.
            //
            //     TODO: Should I create some more getter/setter functions for our page table?
            //           Maybe a getFrame function and a ClearPTE?
            pte_t page = kernel_pt[cur_brk_page_num - i];
            ClearFrame(g_frames,              // frame bit vector pointer
                       page.pfn);             // frame number

            // 6d. Clear the page in the kernel's page table so it is no longer valid
            kernel_pt[cur_brk_page_num - i].valid = 0;
            kernel_pt[cur_brk_page_num - i].prot  = 0;
            kernel_pt[cur_brk_page_num - i].pfn   = 0;
        }
    }

    // 7. Set the kernel brk to the new brk value and return success
    e_kernel_curr_brk = _kernel_new_brk 
    return 0;
}


/*!
 * \desc                 A
 * 
 * \param[in] cmd_args   A
 * \param[in] pmem_size  A
 * \param[in] uctxt      A
 */
void KernelStart(char **cmd_args, unsigned int pmem_size, UserContext *uctxt) {
    // 1. Make sure our user context struct is not NULL and that we
    //    have enough physical memory. If not, halt the machine.
    if (uctxt == NULL || pmem_size < PAGESIZE) {
        Halt();
    }

    // 2. Before we do any dynamic memory allocation (i.e., malloc), make sure that we set our
    //    current brk variable to the incoming original brk, as this is how we will determine
    //    if our brk has changed during our setup process.
    void *e_kernel_curr_brk = _kernel_orig_brk

    // 3. Calculate how many frames we have given our physical memory and page sizes. We will use
    //    a bit vector to track whether a frame is free or in use (0 = free, 1 = used). Calculate
    //    how many bytes we need for our bit vector, given the number of frames and the fact that
    //    a byte is 8 bits.
    //
    //    Remember to check for a remainder---it may be that the number of frames is not divisible
    //    by 8, and C naturally rounds *down*, so we may need to increment our final byte count.
    g_num_frames   = pmem_size    / PAGESIZE;
    int frames_len = g_num_frames / 8;
    if (g_num_frames % 8) {
        frames_len++;
    }

    // 4. Allocate space for our frames bit vector. Use calloc to ensure that the memory is
    //    zero'd out (i.e., that every frame is currently marked as free). Halt upon error.
    g_frames = (char *) calloc(frames_len, sizeof(char));
    if (g_frames == NULL) {
        TracePrintf(1, "Calloc for g_frames failed!\n");
        Halt();
    }

    // 5. Set up the Region 0 page table (i.e., the kernel's page table). Halt upon error.
    pte_t *kernel_pt = (pte_t *) calloc(MAX_PT_LEN, sizeof(pte_t));
    if (kernel_pt == NULL) {
        TracePrintf(1, "Calloc for kernel_pt failed!\n");
        Halt();
    }

    // 6. Calculate the number of pages being used to store the kernel text, which begins at the
    //    start of physical memory and goes up to the beginning of the kernel data region. Since
    //    the kernel text, data, and heap are laid out sequentially in *physical* memory, their
    //    virtual addresses will be exactly the same (i.e., frame and page numbers are the same).
    //    
    //    Configure the page table entries for the text pages with read and execute permissions
    //    (as text contains code), and mark the corresponding physical frames as in use.
    int n_kernel_text_pages = (_kernel_data_start - PMEM_BASE) / PAGESIZE;
    for(int i = 0; i < n_kernel_text_pages; i++) {
        SetPTE(kernel_pt,                // page table pointer
               i,                        // page number
               1,                        // valid bit
               PROT_READ | PROT_EXEC,    // page protection bits
               i);                       // frame number
        
        SetFrame(g_frames,               // frame bit vector pointer
                 i);                     // frame number

    }

    // 7. Calculate the number of pages being used to store the kernel data. Again, since the
    //    kernel is laid out sequentially in physical memory, the page and frame numbers are equal.
    //
    //    Configure the page table entries for the data pages with read and write permissions,
    //    and mark the corresponding physical frames as taken. The data section is above text,
    //    so offset the page/frame numbers by the number of text pages for the correct index.
    int n_kernel_data_pages = (_kernel_data_end - _kernel_data_start) / PAGESIZE;
    for (int i = 0; i < n_kernel_data_pages; i++) {
        SetPTE(kernel_pt,                  // page table pointer
               i + n_kernel_text_pages,    // page number
               1,                          // valid bit
               PROT_READ | PROT_WRITE,     // page protection bits
               i + n_kernel_text_pages);   // frame number
        
        SetFrame(g_frames,                 // frame bit vector pointer
                 i + n_kernel_text_pages); // frame number
    }

    // 8. Tell the CPU where to find our kernel's page table and how large it is
    WriteRegister(REG_PTBR0, kernel_pt);
    WriteRegister(REG_PTLR0, MAX_PT_LEN);

    // 9. Set up the Region 1 page table (i.e., the dummy idle process). Halt upon error.
    pte_t *user_pt = (pte_t *) calloc(MAX_PT_LEN, sizeof(pte_t));
    if (user_pt == NULL) {
        TracePrintf(1, "Calloc for user_pt failed!\n");
        Halt();
    }

    // 10. Find a free frame for the dummy idle process' stack.
    //
    // TODO: IS IT OKAY TO USE THE FRAME ABOVE KERNEL STACK FOR THE FIRST PROCESS'S USER STACK Page?
    //
    // TODO: No, I don't think it is ok to use a frame above the kernel's stack. At least, based on
    //       Sean's response to my ED question, it seems like we can't assume that physical memory
    //       is larger than region 0. So, instead of hardcoding it so that we always choose a frame
    //       above the kernel stack, maybe we should just call our FindFreeFrame function---it may
    //       return a frame within region 0 or it may return a frame in region 1 if we have enough
    //       physical memory for it. Does that make sense?
    int userstack_frame = FindFreeFrame();
    if (userstack_frame == ERROR) {
        TracePrintf(1, "Unable to find free frame for DoIdle userstack!\n");
        Halt();
    }

    SetPTE(user_pt,                   // page table pointer
           userstack_frame,           // page number
           1,                         // valid bit
           PROT_READ | PROT_WRITE,    // page protection bits
           userstack_frame);          // frame number
    
    SetFrame(g_frames,                // frame bit vector pointer
             userstack_frame);        // frame number

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
    //     stack, the max size of which is defined in hardware.h. Use the max stack size to
    //     calculate how many frames are needed. Then, initialize the process' kernel stack
    //     page table, configure its page entries, and mark the corresponding frames as in use.
    int n_ks_frames = KERNEL_STACK_MAXSIZE / PAGESIZE;
    idlePCB->ks_frames = (pte_t *) calloc(n_ks_frames, sizeof(pte_t));
    if (idlePCB->ks_frames == NULL) {
        TracePrintf(1, "Calloc for idlePCB->ks_frames failed!\n");
    }

    int ks_stack_start = KERNEL_STACK_BASE >> PAGESHIFT;
    for (int i = 0; i < n_ks_frames; i++) {
        SetPTE(idlePCB->ks_frames,        // page table pointer
               i,                         // page number
               1,                         // valid bit
               PROT_READ | PROT_WRITE,    // page protection bits
               i + ks_stack_start);       // frame number

        SetFrame(g_frames,                // frame bit vector pointer
                 i + ks_stack_start);     // frame number
    }
    memcpy(&kernel_pt[KERNEL_STACK_BASE >> PAGESHIFT], idlePCB->ksframes, n_ks_frames * sizeof(pte_t));

    // 13. Initialize the user context struct for our dummy idle process. Set the program counter
    //     to point to our DoIdle function, and the stack pointer to the end of region 1 (i.e., to
    //     the top of the stack region we setup in step 12).
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
    int n_kernel_heap_pages = (e_kernel_curr_brk - _kernel_data_end) / PAGESIZE;
    int kernel_heap_start   = n_kernel_text_pages + n_kernel_data_pages;
    for (int i = 0; i < n_kernel_heap_pages; i++) {
        SetPTE(kernel_pt,                   // page table pointer
               i + kernel_heap_start,       // page number
               1,                           // valid bit
               PROT_READ | PROT_WRITE,      // page protection bits
               i + kernel_heap_start);      // frame number
        
        SetFrame(g_frames,                  // frame bit vector pointer
                 i + kernel_heap_start);    // frame number
    }

    // 16. Enable virtual memory!
    g_virtual_memory = 1;
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
 * \desc  A dummy userland process that the kernel runs whenever there are no other processes.
 */
static void DoIdle(void) {
    while(1) {
        TracePrintf(1,"DoIdle\n");
        Pause();
    }
}


/*!
 * \desc    Find a free frame from the physical memory
 * 
 * \return  Frame index on success, ERROR otherwise.
 */
static int FindFreeFrame(void) {
    for (int i = 0; i < g_num_frames; i++) {
        if (TestFrame(g_frames, i) == 0) {
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
static void SetPTE(pte_t *pt, int index, int valid, int prot, int pfn) {
    pt[index].valid = valid;
    pt[index].prot  = prot;
    pt[index].pfn   = pfn;
}
