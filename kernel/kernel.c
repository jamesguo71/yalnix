#include <hardware.h>
#include <yalnix.h>
#include <ykernel.h>

#include "frame.h"
#include "kernel.h"
#include "load_program.h"
#include "scheduler.h"
#include "pte.h"
#include "syscall.h"
#include "trap.h"


/*
 * Extern Global Variable Definitions
 */
char        *e_frames           = NULL;   // Bit vector to track frames (set in KernelStart)
int          e_num_frames       = 0;      // Number of frames           (set in KernelStart)
scheduler_t *e_scheduler        = NULL;
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
    int stack_page_num   = PTEAddressToPage((void *) KERNEL_STACK_BASE);
    int cur_brk_page_num = PTEAddressToPage(e_kernel_curr_brk);
    int new_brk_page_num = PTEAddressToPage(_kernel_new_brk);
    if (new_brk_page_num >= stack_page_num - KERNEL_NUMBER_STACK_FRAMES) {
        TracePrintf(1, "[SetKernelBrk] Error: proposed brk is in red zone.\n");
        return ERROR;
    }

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
            int frame_num = FrameFindAndSet();
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
    //    zero'd out (i.e., that every frame is currently marked as free).
    //    TODO: Should we put frames bit vec and its size inside a small frames_t struct?
    e_frames = (char *) calloc(frames_len, sizeof(char));
    if (!e_frames) {
        TracePrintf(1, "Calloc for e_frames failed!\n");
        Halt();
    }

    // 5. Allocate space for our scheduler struct, which we will use to track processes.
    e_scheduler = SchedulerCreate();
    if (!e_scheduler) {
        TracePrintf(1, "[KernelStart] Failed to create e_scheduler\n");
        Halt();
    }

    // 6. Allocate space for Region 0 page table (i.e., the kernel's page table).
    e_kernel_pt = (pte_t *) calloc(MAX_PT_LEN, sizeof(pte_t));
    if (!e_kernel_pt) {
        TracePrintf(1, "Calloc for e_kernel_pt failed!\n");
        Halt();
    }

    // 7. Create pcbs for our idle and init processes. ProcessCreateIdle will allocate memory for
    //    the Region 1 page table, Region 0 kernel stack page table, and the UserContext. Note that
    //    we do not use ProcessCreate here because it also finds free frames for the process'
    //    kernel stack. Since we have not yet configured our kernel page table, however, this would
    //    find "free" frames that are actually being used for kernel text/data/heap.
    pcb_t *idlePCB = ProcessCreateIdle();
    if (!idlePCB) {
        TracePrintf(1, "[KernelStart] Failed to create idlePCB\n");
        Halt();
    }
    pcb_t *initPCB = ProcessCreateIdle();
    if (!initPCB) {
        TracePrintf(1, "[KernelStart] Failed to create initPCB\n");
        Halt();
    }

    // 8. Configure the UserContext for our idle process. Set its program counter to point to the
    //    DoIdle function and its stack pointer to point to the end of Region 1. The stack grows
    //    downwards, which is why we set the stack pointer to the end of Region 1, BUT be sure to
    //    leave enough room to store a pointer; DoIdle actually gets called by an "init" function
    //    which places a return address on the DoIdle stack---if we set the sp to point to the
    //    last valid byte of Region 1 then when the return address is placed on the stack we will
    //    get a memory fault.
    //
    //    We don't have to set init's UserContext because it gets configured later in LoadProgram.
    idlePCB->uctxt.pc = DoIdle;
    idlePCB->uctxt.sp = (void *) VMEM_1_LIMIT - sizeof(void *);

    // 9. Allocate space for init's KernelContext, but leave idle's NULL. This is because we
    //    plan to run the init program first and have idle clone into init later during a
    //    context switch. More specifically, our context switching code will see that idle's
    //    KC is NULL and call KCCopy to copy the KernelContext of the current running process.
    initPCB->kctxt = (KernelContext *) malloc(sizeof(KernelContext));
    if (!initPCB->kctxt) {
        TracePrintf(1, "[KernelStart] Malloc for initPCB kctxt failed\n");
        Halt();
    }

    // 10. Now that we have finished all of our dynamic memory allocation, we can configure the
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

    // 11. Calculate the number of pages being used to store the kernel data. Again, since the
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

    // 12. Calculate the number of pages being used to store the kernel heap. Again, since the
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

    // 13. Since we plan on running init first, we need to ensure that we map the frames
    //     currently used by the kernel for its stack to inits kernel stack page table.
    //     Since virtual memory is not yet enabled, we know that the kernel stack starting
    //     address maps to the actual frame (and not a virtual page).
    TracePrintf(1, "[KernelStart] Mapping kernel stack pages for init: %d\n", initPCB->pid);
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

    // 14. Find some free frames for idle's kernel stack and map them to the pages in its kernel
    //    stack page table.
    TracePrintf(1, "[KernelStart] Mapping kernel stack pages for idle: %d\n", idlePCB->pid);
    for (int i = 0; i < KERNEL_NUMBER_STACK_FRAMES; i++) {
        int frame = FrameFindAndSet();
        if (frame == ERROR) {
            TracePrintf(1, "[KernelStart] Failed to find a frame for idle kernel stack\n");
            Halt();
        }
        PTESet(idlePCB->ks,                         // page table pointer
               i,                                   // page number
               PROT_READ | PROT_WRITE,              // page protection bits
               frame);                              // frame number
        TracePrintf(1, "[KernelStart] Mapping page: %d to frame: %d\n", i, frame);
    }

    // 15. Find some free frames for idle's userland stack and map them to the pages in its
    //     region 1 page table. Calculate the number of the page that its sp is currently pointing
    //     to, but make sure to subtract MAX_PT_LEN from the result *before* using it to index into
    //     a page table. This is because addresses in region 1 produce page numbers from 128-255,
    //     but our page tables are indexed 0-127.
    //  
    //     Note that LoadProgram would normally LoadProgram perform this step, but we must do it
    //     by hand since DoIdle is not a true userland process---indeed its code lives in the
    //     kernel text (i.e., Region 0)!
    int user_stack_page_num  = (((int ) idlePCB->uctxt.sp) >> PAGESHIFT) - MAX_PT_LEN;
    int user_stack_frame_num = FrameFindAndSet();
    if (user_stack_frame_num == ERROR) {
        TracePrintf(1, "[KernelStart] Unable to find free frame for DoIdle userstack!\n");
        Halt();
    }
    PTESet(idlePCB->pt,               // page table pointer
           user_stack_page_num,       // page number
           PROT_READ | PROT_WRITE,    // page protection bits
           user_stack_frame_num);     // frame number

    // 16. Note that *every* process has a kernel stack, but the kernel only ever uses the one for
    //     the current running process. Thus, whenever we switch processes we need to remember to
    //     copy the process' kernel stack page table into the kernel's master page table.
    //
    //     Since we plan to run init first, we should copy its kernel stack ptes into the master
    //     kernel page table.
    memcpy(&e_kernel_pt[kernel_stack_start_page_num],      // Copy the init proc's page entries
           initPCB->ks,                                    // for its kernel stack into the master
           KERNEL_NUMBER_STACK_FRAMES * sizeof(pte_t));    // kernel page table


    // 17. Tell the CPU where to find our kernel's page table, init's region 1 page table, and our
    //     interrupt vector. Finally, tell the CPU to enable virtual memory and set our virtual
    //     memory flag so that SetKernalBrk knows to treat addresses as virtual from now on.
    WriteRegister(REG_PTBR0,       (unsigned int) e_kernel_pt);         // kernel pt address
    WriteRegister(REG_PTLR0,       (unsigned int) MAX_PT_LEN);          // num entries
    WriteRegister(REG_PTBR1,       (unsigned int) initPCB->pt);         // init pt address
    WriteRegister(REG_PTLR1,       (unsigned int) MAX_PT_LEN);          // num entries
    WriteRegister(REG_VECTOR_BASE, (unsigned int) g_interrupt_table);   // IV address
    WriteRegister(REG_VM_ENABLE, 1);
    g_virtual_memory = 1;

    // 18. We have allocated a pcb and kernel stack for init, but we have not yet loaded it into
    //     memory. If the caller does not specify one, load our default init executable.
    //
    //     LoadProgram will map frames for the region 1 address space and update the process' page
    //     table accordingly. Then it will load the programs code and data into said frames.
    //     Additionally, it will set the brk and data_end variables in inits pcb.
    int ret;
    if (!_cmd_args[0]) {
        ret = LoadProgram("./user/init", _cmd_args, initPCB);
    } else {
        ret = LoadProgram(_cmd_args[0], _cmd_args, initPCB);
    }
    if (ret < 0) {
        TracePrintf(1, "Error loading init program\n");
        Halt();
    }

    // 19. Before we run, add our idle and init pcbs to our scheduler struct, which maintains a 
    //     number of internal lists used to track running, ready, blocked, and other processes.
    //     Mark init as our current running process and add idle to the ready queue.
    //
    //     Afterwards, copy init's UserContext (now configured thanks to LoadProgram) to the
    //     address indicated by _uctxt; this address is where Yalnix will look for the
    //     UserContext of the process that it should currently execute.
    SchedulerAddIdle(e_scheduler,    idlePCB);
    SchedulerAddProcess(e_scheduler, initPCB);
    SchedulerAddRunning(e_scheduler, initPCB);
    memcpy(_uctxt, &initPCB->uctxt, sizeof(UserContext));

    // 20. Print some debugging information for good measure.
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
    TracePrintf(1, "[KernelStart] idlePCB->uctxt.sp:           %p\n", idlePCB->uctxt.sp);
    TracePrintf(1, "[KernelStart] initPCB->uctxt.sp:           %p\n", initPCB->uctxt.sp);
    TracePrintf(1, "[KernelStart] idlePCB->pid:                %d\n", idlePCB->pid);
    TracePrintf(1, "[KernelStart] initPCB->pid:                %d\n", initPCB->pid);
    SchedulerPrintProcess(e_scheduler);
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
int KCSwitch(UserContext *_uctxt, pcb_t *_running_old) {
    // 1. Check if arguments are valid. If not, print message and halt.
    if (!_uctxt) {
        TracePrintf(1, "[KCSwitch] Invalid _uctxt argument\n");
        Halt();
    }

    // 2. Get the next process from our ready queue and mark it as the current running process.
    pcb_t *running_new = SchedulerGetReady(e_scheduler);
    if (!running_new) {
        TracePrintf(1, "[KCSwitch] e_scheduler returned no ready process\n");
        Halt();
    }
    SchedulerAddRunning(e_scheduler, running_new);

    // 3. Before we context switch, check to see if we are actually switching to a new process.
    //    If not, simply return. It may be the case in TrapClock that the current running
    //    process is the only one ready to run because everything else is blocked. If so, we
    //    should just keep running it without wasting time context switching.
    if (_running_old == running_new) {
        return 0;
    }

    // 4. Switch to the new process. If the new process has never been run before, MyKCS will
    //    first call KCCopy to initialize the KernelContext for the new process and clone the
    //    kernel stack contents of the old process.
    int ret = KernelContextSwitch(MyKCS,
                         (void *) _running_old,
                         (void *) running_new);
    if (ret < 0) {
        TracePrintf(1, "[KCSwitch] Failed to switch to the next process\n");
        Halt();
    }

    // 5. At this point, this code is being run by the *new* process, which means that its
    //    running_new stack variable is "stale" (i.e., running_new contains the pcb for the
    //    process that this new process previously gave up the CPU for). Thus, get the
    //    current running process (i.e., "this" process) and set the outgoing _uctxt.
    running_new = SchedulerGetRunning(e_scheduler);
    memcpy(_uctxt, &running_new->uctxt, sizeof(UserContext));
    return 0;
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
    if (!_kctxt || !_next_pcb_p) {
        TracePrintf(1, "[MyKCS] One or more invalid argument pointers\n");
        Halt();
    }

    // 2. Save the incoming KernelContext for the current running process. It is possible that
    //    the current process exited---and its pcb has been free'd---so first check to see that
    //    the pcb still exists. If not, don't bother saving the kernel context.
    pcb_t *running_old = (pcb_t *) _curr_pcb_p;
    if (running_old) {
        memcpy(running_old->kctxt, _kctxt, sizeof(KernelContext));
    }

    // 3. Check to see if the next process has ever been run before. If not, then we need to
    //    initialize the next process' KernelContext and kernel stack contents. Do this by
    //    copying the KernelContext and stack of our init process.
    //
    //    The reason we want to copy init, and not the current running process, is that we know
    //    init's kernel stack simply contains a TrapClock call which will simply return after the
    //    context switch, but the current process may be blocked in a syscall (e.g., wait) that
    //    would cause the new process to get stuck or put in a bad state.
    pcb_t *running_new = (pcb_t *) _next_pcb_p;
    if (!running_new->kctxt) {
        TracePrintf(1, "[MyKCS] Calling KCCopy for pid: %d\n", running_new->pid);
        KCCopy(_kctxt, _next_pcb_p, NULL);
    }

    // 4. Update the kernel's page table so that its stack pages map to
    //    the correct frames for the new running process.
    //
    //    TODO: PTECopy function???
    int kernel_stack_start_page_num = KERNEL_STACK_BASE >> PAGESHIFT;
    memcpy(&e_kernel_pt[kernel_stack_start_page_num],      // Copy the running_new page entries
           running_new->ks,                                // for its kernel stack into the master
           KERNEL_NUMBER_STACK_FRAMES * sizeof(pte_t));    // kernel page table

    // 5. Tell the CPU where to find the page table for our new running process.
    //    Remember to flush the TLB so we dont map to the previous process' frames!
    if (running_old) {
        TracePrintf(1, "[MyKCS] Switching from pid: %d to pid: %d\n",
                                running_old->pid, running_new->pid);
    } else {
        TracePrintf(1, "[MyKCS] Switching from deleted process to pid: %d\n",
                                running_new->pid);
    } 
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