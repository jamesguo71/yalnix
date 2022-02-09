#include "kernel.h"
#include "hardware.h"
#include "trap.h"
#include "yalnix.h"
#include "ykernel.h"

//Bit Manipulation: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/Progs/bit-array2.c
#define BitClear(A,k) ( A[(k/KERNEL_BYTE_SIZE)] &= ~(1 << (k%KERNEL_BYTE_SIZE)) )
#define BitSet(A,k)   ( A[(k/KERNEL_BYTE_SIZE)] |=  (1 << (k%KERNEL_BYTE_SIZE)) )
#define BitTest(A,k)  ( A[(k/KERNEL_BYTE_SIZE)] &   (1 << (k%KERNEL_BYTE_SIZE)) )


/*
 * Extern Global Variable Definitions
 */
int    e_ready_len       = 0;
int    e_blocked_len     = 0;
int    e_terminated_len  = 0;
pcb_t *e_current         = NULL;
pcb_t *e_ready           = NULL;
pcb_t *e_blocked         = NULL;
pcb_t *e_terminated      = NULL;
pte_t *e_kernel_pt       = NULL;
void  *e_kernel_curr_brk = NULL;


/*
 * Local Global Variable Definitions
 */
static char *g_frames         = NULL;   // Bit vector to track frames (set in KernelStart)
static int   g_num_frames     = 0;      // Number of frames           (set in KernelStart)
static int   g_virtual_memory = 0;      // Flag for tracking whether virtual memory is enabled
static void *g_interrupt_table[TRAP_VECTOR_SIZE] = {
    trap_kernel,
    trap_clock,
    trap_illegal,
    trap_memory,
    trap_math,
    trap_tty_receive,
    trap_tty_transmit,
    trap_disk,
    trap_not_handled,
    trap_not_handled,
    trap_not_handled,
    trap_not_handled,
    trap_not_handled,
    trap_not_handled,
    trap_not_handled,
    trap_not_handled,
};


/*
 * Local Function Definitions
 */
static int  FrameClear(int frame_num);
static int  FrameFind(void);
static int  FrameSet(int frame_num);
static int  PTEClear(pte_t *pt, int page_num);
static int  PTESet(pte_t *pt, int page_num, int prot, int pfn);
static void DoIdle(void);


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
    TracePrintf(1, "[SetKernelBrk] _kernel_new_brk: %p\n", _kernel_new_brk);

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
    _kernel_new_brk = (void *) UP_TO_PAGE(_kernel_new_brk);

    // 3. If virtual memory has not yet been enabled, then simply save the proposed brk.
    if (!g_virtual_memory) {
        e_kernel_curr_brk = _kernel_new_brk;
        return 0;
    }

    // 4. If virtual memory has been enabled, then we are responsible for updating the kernel's
    //    page table to reflect any pages/frames that have been added/removed as a result of
    //    the brk change. Start off by calculating the page numbers for our new proposed brk
    //    and our current brk.
    //
    //    Note that we have to do some casting magic here... First cast the void * addresses to
    //    long ints so that we may perform bit operations on them, then we can cast the result
    //    down to an integer as we do not need to top bytes.
    int new_brk_page_num = (int) ((long int) _kernel_new_brk)   >> PAGESHIFT;
    int cur_brk_page_num = (int) ((long int) e_kernel_curr_brk) >> PAGESHIFT;

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
            int frame_num = FrameFind();
            if (frame_num == ERROR) {
                TracePrintf(1, "[SetKernelBrk] Unable to find free frame\n");
                return ERROR;
            }

            // 6b. Map the frame to a page in the kernel's page table. Specifically, start with the
            //     current page pointed to by the kernel brk. Afterwards, mark the frame as in use.
            PTESet(e_kernel_pt,             // page table pointer
                   cur_brk_page_num + i,    // page number
                   PROT_READ | PROT_WRITE,  // page protection bits
                   frame_num);              // frame number
            FrameSet(frame_num);
        } else {
            // 6c. If we are shrinking the heap, then we need to unmap pages. Start by grabbing
            //     the number of the frame mapped to the current brk page. Free the frame.
            //
            //     TODO: Should I create some more getter/setter functions for our page table?
            //           Maybe a getFrame function and a PTEClear?
            int frame_num = e_kernel_pt[cur_brk_page_num - i].pfn;
            FrameClear(frame_num);

            // 6d. Clear the page in the kernel's page table so it is no longer valid
            PTEClear(e_kernel_pt, cur_brk_page_num - i);
        }
    }

    // 7. Set the kernel brk to the new brk value and return success
    e_kernel_curr_brk = _kernel_new_brk;
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
    e_kernel_curr_brk = _kernel_orig_brk;

    // 3. Calculate how many frames we have given our physical memory and page sizes. We will use
    //    a bit vector to track whether a frame is free or in use (0 = free, 1 = used). Calculate
    //    how many bytes we need for our bit vector, given the number of frames and the fact that
    //    a byte is 8 bits.
    //
    //    Remember to check for a remainder---it may be that the number of frames is not divisible
    //    by 8, and C naturally rounds *down*, so we may need to increment our final byte count.
    g_num_frames   = pmem_size    / PAGESIZE;
    int frames_len = g_num_frames / KERNEL_BYTE_SIZE;
    if (g_num_frames % KERNEL_BYTE_SIZE) {
        frames_len++;
    }

    // 4. Allocate space for our frames bit vector. Use calloc to ensure that the memory is
    //    zero'd out (i.e., that every frame is currently marked as free). Halt upon error.
    g_frames = (char *) calloc(frames_len, sizeof(char));
    if (g_frames == NULL) {
        TracePrintf(1, "Calloc for g_frames failed!\n");
        Halt();
    }

    // 5. Allocate space for Region 0 page table (i.e., the kernel's page table). Halt upon error.
    e_kernel_pt = (pte_t *) calloc(MAX_PT_LEN, sizeof(pte_t));
    if (e_kernel_pt == NULL) {
        TracePrintf(1, "Calloc for e_kernel_pt failed!\n");
        Halt();
    }

    // 6. Allocate space for Region 1 page table (i.e., the dummy idle process). Halt upon error.
    pte_t *user_pt = (pte_t *) calloc(MAX_PT_LEN, sizeof(pte_t));
    if (user_pt == NULL) {
        TracePrintf(1, "Calloc for user_pt failed!\n");
        Halt();
    }

    // 7. Allocate space for the PCB of our dummy idle process. Halt upon error.
    //
    //     TODO: We will have to add this to some global array so that the pcb can be
    //           accessed and used during interrupts and syscalls.
    pcb_t *idlePCB = (pcb_t *) malloc(sizeof(pcb_t));
    if (idlePCB == NULL) {
        TracePrintf(1, "[KernelStart] Malloc for idlePCB failed!\n");
        Halt();
    }

    // 8. Allocate space for the dummy idle process kernel stack page table. Halt upon error.
    idlePCB->ks_frames = (pte_t *) calloc(KERNEL_NUMBER_STACK_FRAMES, sizeof(pte_t));
    if (!idlePCB->ks_frames) {
        TracePrintf(1, "[KernelStart] Calloc for idlePCB kernel stack failed!\n");
        Halt();
    }

    // 9. Now that we have finished all of our dynamic memory allocation, we can configure the
    //    kernel's page table (previously, it may have changed due to malloc changing the brk).
    //     
    //    Calculate the number of pages being used to store the kernel text, which begins at the
    //    start of physical memory and goes up to the beginning of the kernel data region. Since
    //    the kernel text, data, and heap are laid out sequentially in *physical* memory, their
    //    virtual addresses will be exactly the same (i.e., frame and page numbers are the same).
    //    
    //    Figure out the page number of the last page in the text region by right-shifting the
    //    last address in the text region by PAGESHIFT bits---this will remove the offset bits
    //    and leave us with the page number for that address.
    //
    //    Configure the page table entries for the text pages with read and execute permissions
    //    (as text contains code), and mark the corresponding physical frames as in use.
    int kernel_text_end_page_num = ((int ) _kernel_data_start) >> PAGESHIFT;
    for(int i = 0; i < kernel_text_end_page_num; i++) {
        PTESet(e_kernel_pt,              // page table pointer
               i,                        // page number
               PROT_READ | PROT_EXEC,    // page protection bits
               i);                       // frame number
        FrameSet(i);
    }

    // 10. Calculate the number of pages being used to store the kernel data. Again, since the
    //     kernel is laid out sequentially in physical memory the page and frame numbers are equal.
    //
    //     Configure the page table entries for the data pages with read and write permissions,
    //     and mark the corresponding physical frames as taken. The data section is above text,
    //     so offset the page/frame numbers by the last text page number.
    int kernel_data_end_page_num = ((int ) _kernel_data_end) >> PAGESHIFT;
    for (int i = kernel_text_end_page_num; i < kernel_data_end_page_num; i++) {
        PTESet(e_kernel_pt,               // page table pointer
               i,                         // page number
               PROT_READ | PROT_WRITE,    // page protection bits
               i);                        // frame number
        FrameSet(i);
    }

    // 11. Calculate the number of pages being used to store the kernel heap. Again, since the
    //     kernel is laid out sequentially in physical memory the page and frame numbers are equal.
    //
    //     Configure the page table entries for the heap pages with read and write permissions,
    //     and mark the corresponding physical frames as taken. The heap section is above data,
    //     so offset the page/frame numbers by the last data page number.
    int kernel_heap_end_page_num = ((int ) e_kernel_curr_brk) >> PAGESHIFT;
    for (int i = kernel_data_end_page_num; i < kernel_heap_end_page_num; i++) {
        PTESet(e_kernel_pt,               // page table pointer
               i,                         // page number
               PROT_READ | PROT_WRITE,    // page protection bits
               i);                        // frame number
        FrameSet(i);
    }

    // 12. Configure the pcb for our dummy idle process. Use the UserContext passed in by the build
    //     system, but change its program counter so that it starts executing at DoIdle.
    // 
    //     The stack grows downwards, so we want to set the stack pointer to the end of Region 1,
    //     BUT be sure to leave enough room to store a pointer; DoIdle actually gets called by an
    //     "init" function which places a return address on the DoIdle stack---if we set the sp to
    //     point to the last valid byte of Region 1 then when the return address is placed on the
    //     stack we will get a memory fault.
    //
    //     Finally, give it the address of its page table and use the build system helper function
    //     to assign the process a pid. Note that the build system keeps a mapping of page table
    //     pointers to pids, so if we don't assign pid via the helper function it complains.
    idlePCB->pt        = user_pt;
    idlePCB->uctxt     = uctxt;
    idlePCB->uctxt->pc = DoIdle;
    idlePCB->uctxt->sp = (void *) VMEM_1_LIMIT - sizeof(void *);
    idlePCB->pid       = helper_new_pid(idlePCB->pt);
    e_current          = idlePCB;

    // 13. Initialize the kernel stack for the dummy idle process. Note that *every* process
    //     has a kernel stack, but that the kernel always looks for its kernel stack at the
    //     end of region 0. Thus, whenever we switch processes we need to remember to copy
    //     the process' kernel stack page table into the kernel's master page table. More
    //     specifically, we need to place the process' page table entries for its kernel
    //     stack at the end of the master kernel page table (i.e., the pages at the end
    //     of Region 0). This way, the kernel always looks at the same pages for its stack
    //     but those pages will map to different physical frames based on the current process.
    //
    //     For our first DoIdle process, however, we can map the last couple of frames in
    //     region 0 to pages of the same number since we know those frames have not been
    //     used yet (i.e., for DoIdle, its kernel stack page number is the same as the
    //     actual frame that it maps to).
    int kernel_stack_start_page_num = KERNEL_STACK_BASE >> PAGESHIFT;
    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        PTESet(idlePCB->ks_frames,                  // page table pointer
               i,                                   // page number
               PROT_READ | PROT_WRITE,              // page protection bits
               i + kernel_stack_start_page_num);    // frame number

        FrameSet(i + kernel_stack_start_page_num);
    }

    memcpy(&e_kernel_pt[kernel_stack_start_page_num],       // Copy the DoIdle proc's page entries
           idlePCB->ks_frames,                              // for its kernel stack into the master
           KERNEL_NUMBER_STACK_FRAMES * sizeof(pte_t));     // kernel page table

    // 14. Initialize the userland stack for the dummy idle process. Since DoIdle doesn't need
    //     a lot of stack space, lets just give it a single page for its stack. Calculate the
    //     number of the page that its sp is currently pointing to, but make sure to subtract
    //     MAX_PT_LEN from the result *before* using it to index into a page table. This is
    //     because addresses in region 1 produce page numbers from 128-255, but our page tables
    //     are indexed 0-127.
    // 
    //     Then, find an available frame to map the userland stack page to; update its
    //     page table and the frame bit vector accordingly.
    int user_stack_page_num  = (((int ) idlePCB->uctxt->sp) >> PAGESHIFT) - MAX_PT_LEN;
    int user_stack_frame_num = FrameFind();
    if (user_stack_frame_num == ERROR) {
        TracePrintf(1, "Unable to find free frame for DoIdle userstack!\n");
        Halt();
    }

    PTESet(user_pt,                   // page table pointer
           user_stack_page_num,       // page number
           PROT_READ | PROT_WRITE,    // page protection bits
           user_stack_frame_num);     // frame number
    FrameSet(user_stack_frame_num);

    // 14. Tell the CPU where to find our kernel's page table, our dummy idle's page table, our
    //     interrupt vector. Finally, tell the CPU to enable virtual memory and set our virtual
    //     memory flag so that SetKernalBrk knows to treat addresses as virtual from now on.
    WriteRegister(REG_PTBR0,       (unsigned int) e_kernel_pt);         // kernel pt address
    WriteRegister(REG_PTLR0,       (unsigned int) MAX_PT_LEN);          // num entries
    WriteRegister(REG_PTBR1,       (unsigned int) user_pt);             // DoIdle pt address
    WriteRegister(REG_PTLR1,       (unsigned int) MAX_PT_LEN);          // num entries
    WriteRegister(REG_VECTOR_BASE, (unsigned int) g_interrupt_table);   // IV address
    WriteRegister(REG_VM_ENABLE, 1);
    g_virtual_memory = 1;

    // 15. Print some debugging information for good measure.
    TracePrintf(1, "[KernelStart] g_num_frames:                %d\n", g_num_frames);
    TracePrintf(1, "[KernelStart] frames_len:                  %d\n", frames_len);
    TracePrintf(1, "[KernelStart] g_frames:                    %p\n", g_frames);
    TracePrintf(1, "[KernelStart] e_kernel_pt:                 %p\n", e_kernel_pt);
    TracePrintf(1, "[KernelStart] user_pt:                     %p\n", user_pt);
    TracePrintf(1, "[KernelStart] kernel_text_end_page_num:    %d\n", kernel_text_end_page_num);
    TracePrintf(1, "[KernelStart] kernel_data_end_page_num:    %d\n", kernel_data_end_page_num);
    TracePrintf(1, "[KernelStart] kernel_heap_end_page_num:    %d\n", kernel_heap_end_page_num);
    TracePrintf(1, "[KernelStart] kernel_stack_start_page_num: %d\n", kernel_stack_start_page_num);
    TracePrintf(1, "[KernelStart] user_stack_page_num:         %d\n", user_stack_page_num);
    TracePrintf(1, "[KernelStart] user_stack_frame_num:        %d\n", user_stack_frame_num);
    TracePrintf(1, "[KernelStart] idlePCB->uctxt->sp:          %p\n", idlePCB->uctxt->sp);

    // Check if cmd_args are blank. If blank, kernel starts to look for a executable called “init”.
    // Otherwise, load `cmd_args[0]` as its initial process.

    // For Checkpoint 3, KernelStart needs to create an initPCB and Set up the identity for the new initPCB
    // Then write KCCopy() to: copy the current KernelContext into initPCB and
    // copy the contents of the current kernel stack into the new kernel stack frames in initPCB
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
    // // 1. Check arguments. Return error if invalid.
    // if (!kc_in || !new_pcb_p) {
    //     return NULL;
    // }

    // // 2. Cast our new process pcb pointer so that we can reference the internal variables.
    // pcb_t *pcb = (pcb_t *) new_pcb_p;

    // // 3. Copy the incoming KernelContext into the new process' pcb. Can I use memcpy
    // //    in the kernel or do I need to implement it myself (i.e., use void pointers
    // //    and loop over copying byte-by-byte)? I bet I have to do it manually...
    // memcpy(pcb->kctxt, kc_in, sizeof(KernelContext));

    // // 4. I'm supposed to copy the current kernel stack frames over to the new process' pcb, but
    // //    I do not understand how I access the current kernel stack frames with only kc_in. Is
    // //    it something related to kstack_cs? What is that variable?
    // for (int i = 0; i < numFrames; i++) {
    //     // copy frames over somehow???
    // }

    // // x. Return the incoming KernelContext
    // return kc_in
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
 * \desc                 Marks the frame indicated by "frame_num" as free by clearing its bit
 *                       in our global frame bit vector.
 * 
 * \param[in] frame_num  The number of the frame to free
 * 
 * \return               0 on success, ERROR otherwise.
 */
static int FrameClear(int frame_num) {
    // 1. Check that our frame number is valid. If not, print message and return error.
    if (frame_num < 0 || frame_num >= g_num_frames) {
        TracePrintf(1, "[FrameClear] Invalid frame number: %d\n", frame_num);
        return ERROR;
    }

    // 2. Check to see if the frame indicated by frame_num is already invalid.
    //    If so, print a warning message but do not return ERROR.
    if(!BitTest(g_frames, frame_num)) {
        TracePrintf(1, "[FrameClear] Warning: frame %d is already invalid\n", frame_num);
    }

    // 3. "Free" the frame indicated by frame_num by clearing its bit in our frames bit vector.
    BitClear(g_frames, frame_num);
    return 0;
}


/*!
 * \desc    Find a free frame from the physical memory
 * 
 * \return  Frame number on success, ERROR otherwise.
 */
static int FrameFind(void) {
    for (int i = 0; i < g_num_frames; i++) {
        if (BitTest(g_frames, i) == 0) {
            return i;
        }
    }
    return ERROR;
}


/*!
 * \desc                 Marks the frame indicated by "frame_num" as in use by setting its bit
 *                       in our global frame bit vector.
 * 
 * \param[in] frame_num  The number of the frame to mark as in use
 * 
 * \return               0 on success, ERROR otherwise.
 */
static int FrameSet(int frame_num) {
    // 1. Check that our frame number is valid. If not, print message and return error.
    if (frame_num < 0 || frame_num >= g_num_frames) {
        TracePrintf(1, "[FrameSet] Invalid frame number: %d\n", frame_num);
        return ERROR;
    }

    // 2. Check to see if the frame indicated by frame_num is already valid.
    //    If so, print a warning message but do not return ERROR.
    if(BitTest(g_frames, frame_num)) {
        TracePrintf(1, "[FrameSet] Warning: frame %d is already valid\n", frame_num);
    }

    // 3. Mark the frame indicated by frame_num as in use by
    //    setting its bit in the frame bit vector.
    BitSet(g_frames, frame_num);
    return 0;
}


/*!
 * \desc             A
 * 
 * \param[in] pt     T
 * \param[in] index  T
 * 
 * \return  Frame index on success, ERROR otherwise.
 */
static int PTEClear(pte_t *pt, int page_num) {
    // 1. Check that our page table pointer is invalid. If not, print message and return error.
    if (!pt) {
        TracePrintf(1, "[PTEClear] Invalid page table pointer\n");
        return ERROR;
    }

    // 2. Check that our page number is valid. If not, print message and return error.
    if (page_num < 0 || page_num >= MAX_PT_LEN) {
        TracePrintf(1, "[PTEClear] Invalid page number: %d\n", page_num);
        return ERROR;
    }

    // 3. Check to see if the entry indicated by page_num is already invalid.
    //    If so, print a warning message, but do not return ERROR. 
    if (!pt[page_num].valid) {
        TracePrintf(1, "[PTEClear] Warning: page %d is alread invalid\n", page_num);
    }

    // 4. Mark the page table entry indicated by page_num as invalid.
    pt[page_num].valid = 0;
    pt[page_num].prot  = 0;
    pt[page_num].pfn   = 0;
    return 0;
}


/*!
 * \desc                A
 * 
 * \param[in] pt        T
 * \param[in] page_num  T
 * \param[in] prot      T
 * \param[in] pfn       T
 * 
 * \return  Frame index on success, ERROR otherwise.
 */
static int PTESet(pte_t *pt, int page_num, int prot, int pfn) {
    // 1. Check that our page table pointer is invalid. If not, print message and return error.
    if (!pt) {
        TracePrintf(1, "[PTESet] Invalid page table pointer\n");
        return ERROR;
    }

    // 2. Check that our page number is valid. If not, print message and return error.
    if (page_num < 0 || page_num >= MAX_PT_LEN) {
        TracePrintf(1, "[PTESet] Invalid page number: %d\n", page_num);
        return ERROR;
    }

    // 3. Check that our frame number is valid. If not, print message and return error.
    if (pfn < 0 || pfn >= g_num_frames) {
        TracePrintf(1, "[PTESet] Invalid frame number: %d\n", page_num);
        return ERROR;
    }

    // 4. Check to see if the entry indicated by page_num is already valid.
    //    If so, print a warning message, but do not return ERROR. 
    if (pt[page_num].valid) {
        TracePrintf(1, "[PTESet] Warning: page %d is alread valid\n", page_num);
    }

    // 5. Make the page table entry indicated by page_num valid, set
    //    its protections and map it to the frame indicated by pfn.
    pt[page_num].valid = 1;
    pt[page_num].prot  = prot;
    pt[page_num].pfn   = pfn;
    return 0;
}


/*!
 * \desc  A dummy userland process that the kernel runs when there are no other processes.
 */
static void DoIdle(void) {
    while(1) {
        TracePrintf(1,"DoIdle\n");
        Pause();
    }
}