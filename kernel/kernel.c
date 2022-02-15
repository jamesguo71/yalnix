#include <hardware.h>
#include <yalnix.h>
#include <ykernel.h>

#include "frame.h"
#include "kernel.h"
#include "load_program.h"
#include "proc_list.h"
#include "pte.h"
#include "syscall.h"
#include "trap.h"


/*
 * Extern Global Variable Definitions
 */
char        *e_frames           = NULL;   // Bit vector to track frames (set in KernelStart)
int          e_num_frames       = 0;      // Number of frames           (set in KernelStart)
proc_list_t *e_proc_list        = NULL;
pte_t       *e_kernel_pt        = NULL;
void        *e_kernel_curr_brk  = NULL;


/*
 * Local Global Variable Definitions
 */
static int   g_virtual_memory = 0;      // Flag for tracking whether virtual memory is enabled
static void *g_interrupt_table[TRAP_VECTOR_SIZE] = {
    TrapKernel,
    TrapClock,
    TrapIllegal,
    TrapMemory,
    TrapMath,
    TrapTTYReceive,
    TrapTTYTransmit,
    TrapDisk,
    TrapNotHandled,
    TrapNotHandled,
    TrapNotHandled,
    TrapNotHandled,
    TrapNotHandled,
    TrapNotHandled,
    TrapNotHandled,
    TrapNotHandled,
};


/*
 * Local Function Definitions
 */
static void DoIdle(void);
static void DoIdle2(void);


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
    int new_brk_page_num = ((int) _kernel_new_brk)   >> PAGESHIFT;
    int cur_brk_page_num = ((int) e_kernel_curr_brk) >> PAGESHIFT;

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
            TracePrintf(1, "[SetKernelBrk] Mapping page: %d to frame: %d\n",
                           cur_brk_page_num + i, frame_num);
        } else {
            // 6c. If we are shrinking the heap, then we need to unmap pages. Start by grabbing
            //     the number of the frame mapped to the current brk page. Free the frame.
            int frame_num = e_kernel_pt[cur_brk_page_num - i].pfn;
            FrameClear(frame_num);

            // 6d. Clear the page in the kernel's page table so it is no longer valid
            PTEClear(e_kernel_pt, cur_brk_page_num - i);
            TracePrintf(1, "[SetKernelBrk] Unmapping page: %d from frame: %d\n",
                           cur_brk_page_num - i, frame_num);
        }
    }

    // 7. Set the kernel brk to the new brk value and return success
    TracePrintf(1, "[SetKernelBrk] new_brk_page_num:  %d\n", new_brk_page_num);
    TracePrintf(1, "[SetKernelBrk] cur_brk_page_num:  %d\n", cur_brk_page_num);
    TracePrintf(1, "[SetKernelBrk] _kernel_new_brk:   %p\n", _kernel_new_brk);
    TracePrintf(1, "[SetKernelBrk] e_kernel_curr_brk: %p\n", e_kernel_curr_brk);
    e_kernel_curr_brk = _kernel_new_brk;
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    return 0;
}


/*!
 * \desc                 Initializes kernel variables and structures needs to track processes,
 *                       locks, cvars, locks, and other OS related functionalities. Additionally,
 *                       we setup the kernel's page table and a dummy "DoIdle" process to run
 *                       when no other processes are available.
 * 
 * \param[in] cmd_args   Not used for checkpoint 2
 * \param[in] pmem_size  The size of the physical memory availabe to our system (in bytes)
 * \param[in] uctxt      An initialized usercontext struct for the DoIdle process
 */
void KernelStart(char **_cmd_args, unsigned int _pmem_size, UserContext *_uctxt) {
    // 1. Make sure our user context struct is not NULL and that we
    //    have enough physical memory. If not, halt the machine.
    if (!_uctxt || _pmem_size < PAGESIZE) {
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
    e_num_frames   = _pmem_size   / PAGESIZE;
    int frames_len = e_num_frames / KERNEL_BYTE_SIZE;
    if (e_num_frames % KERNEL_BYTE_SIZE) {
        frames_len++;
    }

    // 4. Allocate space for our frames bit vector. Use calloc to ensure that the memory is
    //    zero'd out (i.e., that every frame is currently marked as free). Halt upon error.
    e_frames = (char *) calloc(frames_len, sizeof(char));
    if (!e_frames) {
        TracePrintf(1, "Calloc for e_frames failed!\n");
        Halt();
    }

    // 5. Allocate space for the kernel list structures, which are used to track processes,
    //    locks, cvars, and pipes. Halt upon error if any of these initializations fail.
    e_proc_list = ProcListCreate();
    if (!e_proc_list) {
        TracePrintf(1, "[KernelStart] Error allocating e_proc_list\n");
        Halt();
    }

    // 6. Allocate space for Region 0 page table (i.e., the kernel's page table). Halt upon error.
    e_kernel_pt = (pte_t *) calloc(MAX_PT_LEN, sizeof(pte_t));
    if (!e_kernel_pt) {
        TracePrintf(1, "Calloc for e_kernel_pt failed!\n");
        Halt();
    }

    // 7. Allocate space for the idle and init PCBs, their region 1 page tables, and
    //    their region 0 stack tables. Print message and halt machine upon error.
    pcb_t *idlePCB = (pcb_t *) malloc(sizeof(pcb_t));
    if (!idlePCB) {
        TracePrintf(1, "[KernelStart] Malloc for idlePCB failed!\n");
        Halt();
    }
    pcb_t *initPCB = (pcb_t *) malloc(sizeof(pcb_t));
    if (!initPCB) {
        TracePrintf(1, "[KernelStart] Malloc for initPCB failed!\n");
        Halt();
    }

    // Region 1 tables - need entries for the full address space
    idlePCB->pt = (pte_t *) calloc(MAX_PT_LEN, sizeof(pte_t));
    if (!idlePCB->pt) {
        TracePrintf(1, "Calloc for idlePCB->pt failed!\n");
        Halt();
    }
    initPCB->pt = (pte_t *) calloc(MAX_PT_LEN, sizeof(pte_t));
    if (!initPCB->pt) {
        TracePrintf(1, "Calloc for initPCB->pt failed!\n");
        Halt();
    }

    // Region 0 stack tables - only need space to hold enough entries for the stack
    idlePCB->ks = (pte_t *) calloc(KERNEL_NUMBER_STACK_FRAMES, sizeof(pte_t));
    if (!idlePCB->ks) {
        TracePrintf(1, "[KernelStart] Calloc for idlePCB kernel stack failed!\n");
        Halt();
    }
    initPCB->ks = (pte_t *) calloc(KERNEL_NUMBER_STACK_FRAMES, sizeof(pte_t));
    if (!initPCB->ks) {
        TracePrintf(1, "[KernelStart] Calloc for initPCB kernel stack failed!\n");
        Halt();
    }

    // 8. Configure the pcb for our idle process. Allocate space for its KernelContext (which will
    //    be initialized later in MyKCS), and its UserContext. Set its program counter to point to
    //    the DoIdle function and its stack pointer to point to the end of Region 1.
    // 
    //    The stack grows downwards, so we want to set the stack pointer to the end of Region 1,
    //    BUT be sure to leave enough room to store a pointer; DoIdle actually gets called by an
    //    "init" function which places a return address on the DoIdle stack---if we set the sp to
    //    point to the last valid byte of Region 1 then when the return address is placed on the
    //    stack we will get a memory fault.
    //
    //    Copy the context of our configured UserContext to the address indicated by _uctxt; this
    //    address is where Yalnix will look for the UserContext of the current executing process.
    // idlePCB->kctxt     = (KernelContext *) malloc(sizeof(KernelContext));
    idlePCB->kctxt     = NULL;
    idlePCB->uctxt     = (UserContext   *) malloc(sizeof(UserContext));
    idlePCB->uctxt->pc = DoIdle;
    idlePCB->uctxt->sp = (void *) VMEM_1_LIMIT - sizeof(void *);

    // 9. Configure our init pcb the same way, except do NOT allocate space for KernelContext as
    //    our KCSwitch function uses the presence of NULL to determine if KCCopy should be called.
    //    More specifically, KCCopy will be used to initialize KernelContext the first time that
    //    the init process gets to run.
    initPCB->kctxt     = (KernelContext *) malloc(sizeof(KernelContext));
    initPCB->uctxt     = (UserContext *) malloc(sizeof(UserContext));
    initPCB->brk       = NULL;
    initPCB->data_end  = NULL;
    initPCB->text_end  = NULL;

    // 10. Assign our processes pids with the build system helper function. Note that the build
    //     system keeps a mapping of page table pointers to pids, so if we don't assign pid via
    //     the helper function it complains about the PTBR1 not being assigned to a process
    //
    //     Add our pcbs to our process list structure for tracking. Note that we add idle as the
    //     current runny process, whereas init gets put into the ready queue.
    initPCB->pid = helper_new_pid(initPCB->pt);
    idlePCB->pid = helper_new_pid(idlePCB->pt);
    ProcListProcessAdd(e_proc_list, initPCB);
    ProcListProcessAdd(e_proc_list, idlePCB);
    ProcListRunningSet(e_proc_list, initPCB);
    ProcListReadyAdd(e_proc_list,   idlePCB);


    // 11. Now that we have finished all of our dynamic memory allocation, we can configure the
    //     kernel's page table (previously, it may have changed due to malloc changing the brk).
    //     
    //     Calculate the number of pages being used to store the kernel text, which begins at the
    //     start of physical memory and goes up to the beginning of the kernel data region. Since
    //     the kernel text, data, and heap are laid out sequentially in *physical* memory, their
    //     virtual addresses will be exactly the same (i.e., frame and page numbers are the same).
    //    
    //     Figure out the page number of the last page in the text region by right-shifting the
    //     last address in the text region by PAGESHIFT bits---this will remove the offset bits
    //     and leave us with the page number for that address.
    //
    //     Configure the page table entries for the text pages with read and execute permissions
    //     (as text contains code), and mark the corresponding physical frames as in use.
    int kernel_text_end_page_num = ((int ) _kernel_data_start) >> PAGESHIFT;
    for(int i = 0; i < kernel_text_end_page_num; i++) {
        PTESet(e_kernel_pt,              // page table pointer
               i,                        // page number
               PROT_READ | PROT_EXEC,    // page protection bits
               i);                       // frame number
        FrameSet(i);
    }

    // 12. Calculate the number of pages being used to store the kernel data. Again, since the
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

    // 13. Calculate the number of pages being used to store the kernel heap. Again, since the
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

    // 14. Initialize the kernel stack for the dummy idle process. Note that *every* process
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
    TracePrintf(1, "[KernelStart] Mapping kernel stack pages for init\n");
    int kernel_stack_start_page_num = KERNEL_STACK_BASE >> PAGESHIFT;
    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        PTESet(initPCB->ks,                         // page table pointer
               i,                                   // page number
               PROT_READ | PROT_WRITE,              // page protection bits
               i + kernel_stack_start_page_num);    // frame number
        FrameSet(i + kernel_stack_start_page_num);
        TracePrintf(1, "[KernelStart] Mapping page: %d to frame: %d\n",
                      i,
                      i + kernel_stack_start_page_num);
    }
    memcpy(&e_kernel_pt[kernel_stack_start_page_num],      // Copy the DoIdle proc's page entries
           initPCB->ks,                                    // for its kernel stack into the master
           KERNEL_NUMBER_STACK_FRAMES * sizeof(pte_t));    // kernel page table

    TracePrintf(1, "[KernelStart] Mapping kernel stack pages for idle\n");
    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        int frame = FrameFind();
        PTESet(idlePCB->ks,                         // page table pointer
               i,                                   // page number
               PROT_READ | PROT_WRITE,              // page protection bits
               frame);    // frame number

        FrameSet(frame);
        TracePrintf(1, "[KernelStart] Mapping page: %d to frame: %d\n",
                    i,
                    frame);
    }

    // 15. Initialize the userland stack for the dummy idle process. Since DoIdle doesn't need
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
    PTESet(idlePCB->pt,               // page table pointer
           user_stack_page_num,       // page number
           PROT_READ | PROT_WRITE,    // page protection bits
           user_stack_frame_num);     // frame number
    FrameSet(user_stack_frame_num);

    // 16. Tell the CPU where to find our kernel's page table, our dummy idle's page table, our
    //     interrupt vector. Finally, tell the CPU to enable virtual memory and set our virtual
    //     memory flag so that SetKernalBrk knows to treat addresses as virtual from now on.
    WriteRegister(REG_PTBR0,       (unsigned int) e_kernel_pt);         // kernel pt address
    WriteRegister(REG_PTLR0,       (unsigned int) MAX_PT_LEN);          // num entries
    WriteRegister(REG_PTBR1,       (unsigned int) initPCB->pt);         // DoIdle pt address
    WriteRegister(REG_PTLR1,       (unsigned int) MAX_PT_LEN);          // num entries
    WriteRegister(REG_VECTOR_BASE, (unsigned int) g_interrupt_table);   // IV address
    WriteRegister(REG_VM_ENABLE, 1);
    g_virtual_memory = 1;

    int ret = LoadProgram(_cmd_args[0], _cmd_args, initPCB);
    if (ret < 0) {
        TracePrintf(1, "Error loading init program\n");
        Halt();
    }
    memcpy(_uctxt, initPCB->uctxt, sizeof(UserContext));

    // 17. Print some debugging information for good measure.
    TracePrintf(1, "[KernelStart] e_num_frames:                %d\n", e_num_frames);
    TracePrintf(1, "[KernelStart] e_frames:                    %p\n", e_frames);
    TracePrintf(1, "[KernelStart] e_kernel_pt:                 %p\n", e_kernel_pt);
    TracePrintf(1, "[KernelStart] idlePCB->pt:                 %p\n", idlePCB->pt);
    TracePrintf(1, "[KernelStart] initPCB->pt:                 %p\n", initPCB->pt);
    TracePrintf(1, "[KernelStart] kernel_text_end_page_num:    %d\n", kernel_text_end_page_num);
    TracePrintf(1, "[KernelStart] kernel_data_end_page_num:    %d\n", kernel_data_end_page_num);
    TracePrintf(1, "[KernelStart] kernel_heap_end_page_num:    %d\n", kernel_heap_end_page_num);
    TracePrintf(1, "[KernelStart] kernel_stack_start_page_num: %d\n", kernel_stack_start_page_num);
    TracePrintf(1, "[KernelStart] idle_stack_frame_num:        %d\n", user_stack_frame_num);
    TracePrintf(1, "[KernelStart] idlePCB->uctxt->sp:          %p\n", idlePCB->uctxt->sp);
    TracePrintf(1, "[KernelStart] initPCB->uctxt->sp:          %p\n", initPCB->uctxt->sp);
    TracePrintf(1, "[KernelStart] idlePCB->pid:                %d\n", idlePCB->pid);
    TracePrintf(1, "[KernelStart] initPCB->pid:                %d\n", initPCB->pid);
    ProcListProcessPrint(e_proc_list);
    // Check if cmd_args are blank. If blank, kernel starts to look for a executable called “init”.
    // Otherwise, load `cmd_args[0]` as its initial process.
}


/*!
 * \desc                 Small wrapper for Kernel Context Switching that will first call KCCopy if
 *                       the new process does not yet have a KernelContext.
 * 
 * \param[in] kc_in       a pointer to a temporary copy of the current kernel context of the caller
 * \param[in] curr_pcb_p  a void pointer to current process's pcb
 * \param[in] next_pcb_p  a void pointer to the next process's pcb
 * 
 * \return               Returns the kc_in pointer
 */
KernelContext *KCSwitch(KernelContext *_kctxt, void *_curr_pcb_p, void *_next_pcb_p) {
    // 1. Check if arguments are valid. If not, print message and halt.
    if (!_kctxt || !_curr_pcb_p || !_next_pcb_p) {
        TracePrintf(1, "[KCSwitch] One or more invalid argument pointers\n");
        Halt();
    }

    // 2. Check to see if the next process has ever been run before. If not, call KCCopy to copy
    //    the current process' KernelContext along with the contents of its stack frames.
    pcb_t *running_old = (pcb_t *) _curr_pcb_p;
    pcb_t *running_new = (pcb_t *) _next_pcb_p;
    if (!running_new->kctxt) {
        TracePrintf(1, "[KCSwitch] Calling KCCopy for pid: %d\n", running_new->pid);
        KCCopy(_kctxt, _next_pcb_p, NULL);
    }

    // 3. Switch from the old process to the new
    return MyKCS(_kctxt, _curr_pcb_p, _next_pcb_p);
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
KernelContext *KCCopy(KernelContext *_kctxt, void *_new_pcb_p, void *_not_used) {
    // 1. Check if arguments are valid. If not, print message and halt.
    if (!_kctxt || !_new_pcb_p) {
        TracePrintf(1, "[MyKCCopy] One or more invalid argument pointers\n");
        Halt();
    }

    // 2. Cast our void arguments to our custom pcb struct. Allocate space for
    //    a KernelContext then copy over the incoming context. Halt upon error.
    pcb_t *running_new = (pcb_t *) _new_pcb_p;
    running_new->kctxt = (KernelContext *) malloc(sizeof(KernelContext));
    if (!running_new->kctxt) {
        TracePrintf(1, "[MyKCCopy] Error allocating space for KernelContext\n");
        Halt();
    }
    memcpy(running_new->kctxt, _kctxt, sizeof(KernelContext));
    TracePrintf(1, "[KCCopy] Copying KernelContext for pid: %d\n", running_new->pid);

    // 3. This is where things get tricky. We need to copy the contents of the stack for our
    //    current process over to the stack for our new process. The reason this is tricky, is
    //    that the frames for our new process are not mapped into the current process' address
    //    space. In other words, our virtual addresses will always get translated and mapped to
    //    stack frames for the current process, not the new one.
    //
    //    Thus, we need to temporarilly map the new process' stack frames into the current space.
    //    Calculate the starting page number for the current process' stack, then subtract enough
    //    pages to put our temporary copy beneath the current process' stack.
    //
    //    Update our kernel's page table by mapping the mapping our temporary pages to the
    //    new process' stack frames.
    int kernel_stack_start_page_num = KERNEL_STACK_BASE >> PAGESHIFT;
    int kernel_stack_temp_page_num  = kernel_stack_start_page_num - KERNEL_NUMBER_STACK_FRAMES;
    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        PTESet(e_kernel_pt,                     // page table pointer
               i + kernel_stack_temp_page_num,  // page number
               PROT_READ | PROT_WRITE,          // page protection bits
               running_new->ks[i].pfn);         // frame number
    }

    // 4. Calculate the address for the starting byte of the current process stack and
    //    for our temporary stack. Then, copy over the contents of our current process
    //    stack to the temporary stack---remember, the temporary pages are mapped to
    //    the new process' stack frames, so even though these pages are temporary for
    //    the current process the new process still has a reference to the stack frames.
    void *kernel_stack_start_addr = (void *) (kernel_stack_start_page_num << PAGESHIFT);
    void *kernel_stack_temp_addr  = (void *) (kernel_stack_temp_page_num  << PAGESHIFT);
    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        memcpy(kernel_stack_temp_addr, kernel_stack_start_addr, PAGESIZE);
        kernel_stack_start_addr += PAGESIZE;
        kernel_stack_temp_addr += PAGESIZE;
    }

    // 5. Unmap the temporary stack pages from the next processes stack frames.
    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        PTEClear(e_kernel_pt, i + kernel_stack_temp_page_num);
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    return _kctxt;
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
KernelContext *MyKCS(KernelContext *_kctxt, void *_curr_pcb_p, void *_next_pcb_p) {
    // 1. Check if arguments are valid. If not, print message and halt.
    if (!_kctxt || !_curr_pcb_p || !_next_pcb_p) {
        TracePrintf(1, "[MyKCS] One or more invalid argument pointers\n");
        Halt();
    }

    // 2. Cast our void arguments to our custom pcb struct. Save the incoming KernelContext
    //    for the current running process that we are about to switch out.
    pcb_t *running_new = (pcb_t *) _next_pcb_p;
    pcb_t *running_old = (pcb_t *) _curr_pcb_p;
    memcpy(running_old->kctxt, _kctxt, sizeof(KernelContext));
    TracePrintf(1, "[MyKCS] Switching from pid: %d to pid: %d\n", running_old->pid, running_new->pid);

    // 3. Update the kernel's page table so that its stack pages map to
    //    the correct frames for the new running process.
    //
    //    TODO: PTECopy function???
    int kernel_stack_start_page_num = KERNEL_STACK_BASE >> PAGESHIFT;
    memcpy(&e_kernel_pt[kernel_stack_start_page_num],      // Copy the running_new page entries
           running_new->ks,                                // for its kernel stack into the master
           KERNEL_NUMBER_STACK_FRAMES * sizeof(pte_t));    // kernel page table

    // 4. Tell the CPU where to find the page table for our new running process.
    //    Remember to flush the TLB so we dont map to the previous process' frames!
    WriteRegister(REG_PTBR1, (unsigned int) running_new->pt);    // pt address
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    return running_new->kctxt;
}


/*!
 * \desc  A dummy userland process that the kernel runs when there are no other processes.
 */
static void DoIdle(void) {
    while(1) {
        TracePrintf(1,"DoIdle 1\n");
        Pause();
    }
}

/*!
 * \desc  A dummy userland process that the kernel runs when there are no other processes.
 */
static void DoIdle2(void) {
    while(1) {
        TracePrintf(1,"DoIdle 2\n");
        Pause();
    }
}