# Yalnix - Team Unics
This repository contains our (Fei & Taylor) code for the COSC258 Yalnix project.


## Checkpoint 1: Pseudocode
The yalnix guide states that for our first checkpoint we need to (1) sketch all of our kernel data structures and (2) write pseudocode for all of our traps, syscalls, and major library functions. Below are the data structures and functions that I *think* we need to provide.

*TODO: Add a table of contents that links to each data structure and kernel function?*


### Kernel Structures
Below are descriptions and pseudocode for the structures that we think we will need to implement our Yalnix kernel.

#### Process Control Block (PCB)
*struct description here*

```c
typedef struct pcb {
    int pid;

    void *pc;       // points to current instruction
    void *sp;       // points to current stack frame

    void *data;     // points to bottom of global data memory segment
    void *heap;     // points to bottom of heap memory segment
    void *stack;    // points to top of stack memory segment
    void *text;     // points to bottom of text memory segment

    pcb_t *children;

    pt_t *pageTable;
} pcb_t
```

> Based on Page 71
```c
typedef struct pcb {
    int pid;
    UserContext *uctxt; // Defined in `hardware.h`    
    pte_t *ks_frames;
    KernelContext *kctxt; // Needed for KernelCopy? See Page 45 
    pte_t *pt;          // Defined in `hardware.h`
} pcb_t;
```

#### Process scheduling

```c
pcb_t *ready_queue;
pcb_t *blocked;

```

#### Struct Name Here
*struct description here*

```c
// Struct code here
```


### Kernel Functions
Below are descriptions and pseudocode for the functions that we think we will need to implement our Yalnix kernel.

#### SetKernelBrk
*Look in `hardware.h` for descriptions of the kernel functions*

```c
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
```

#### KernelStart

The hardware invokes KernelStart() to boot the system. When this function returns, the machine begins running in user mode at a specified UserContext.

```c
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
```

#### MyKCS

MyKCS will be called by `KernelContextSwitch` and fed with a pointer to a temporary copy of the current kernel context, and these two PCB pointers.

```c
KernelContext *MyKCS(KernelContext *, void *, void *)
    // Copy the kernel context into the current process’s PCB and return a pointer to a kernel context it had earlier saved in the next process’s PCB.
```

#### KCCopy

KCCopy will simply copy the kernel context from `*kc_in` into the new pcb, and copy the contents of the current kernel stack into the frames that have been allocated for the new process’s kernel stack. However, it will then return `kc_in`.

```c
KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Nop
*function description here*

```c
int Nop (int,int,int,int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Fork

```c
int Fork (void) {
    // Create a new pcb for child process
    // Get a new pid for the child process
    // Copy user_context into the new pcb
    // Call KernelContextSwitch(KCCopy, *new_pcb, NULL) to copy the current process into the new pcb
    // 

}
```

#### Exec
*function description here*

```c
int Exec (char *, char **) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Exit
*function description here*

```c
void Exit (int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Wait
*function description here*

```c
int Wait (int *) {
    // 1. Check arguments. Return error if invalid.
}
```

#### GetPid
*function description here*

```c
int GetPid (void) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Brk
*function description here*

```c
int Brk (void *) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Delay
*function description here*

```c
int Delay (int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### TtyRead
*function description here*

```c
int TtyRead (int, void *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### TtyWrite
*function description here*

```c
int TtyWrite (int, void *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Register
*function description here*

```c
int Register (unsigned int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Send
*function description here*

```c
int Send (void *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Receive
*function description here*

```c
int Receive (void *) {
    // 1. Check arguments. Return error if invalid.
}
```

#### ReceiveSpecific
*function description here*

```c
int ReceiveSpecific (void *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Reply
*function description here*

```c
int Reply (void *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Forward
*function description here*

```c
int Forward (void *, int, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### CopyFrom
*function description here*

```c
int CopyFrom (int, void *, void *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### CopyTo
*function description here*

```c
int CopyTo (int, void *, void *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### ReadSector
*function description here*

```c
int ReadSector (int, void *) {
    // 1. Check arguments. Return error if invalid.
}
```

#### WriteSector
*function description here*

```c
int WriteSector (int, void *) {
    // 1. Check arguments. Return error if invalid.
}
```

#### PipeInit
*function description here*

```c
int PipeInit (int *) {
    // 1. Check arguments. Return error if invalid.
}
```

#### PipeRead
*function description here*

```c
int PipeRead (int, void *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### PipeWrite
*function description here*

```c
int PipeWrite (int, void *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### SemInit
*function description here*

```c
int SemInit (int *, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### SemUp
*function description here*

```c
int SemUp (int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### SemDown
*function description here*

```c
int SemDown (int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### LockInit
*function description here*

```c
int LockInit (int *) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Acquire
*function description here*

```c
int Acquire (int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Release
*function description here*

```c
int Release (int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### CvarInit
*function description here*

```c
int CvarInit (int *) {
    // 1. Check arguments. Return error if invalid.
}
```

#### CvarWait
*function description here*

```c
int CvarWait (int, int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### CvarSignal
*function description here*

```c
int CvarSignal (int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### CvarBroadcast
*function description here*

```c
int CvarBroadcast (int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Reclaim
*function description here*

```c
int Reclaim (int) {
    // 1. Check arguments. Return error if invalid.
}
```

#### Function Name Here
*function description here*

```c
// function code here
```

Here is a list of the different types of "traps" that Yalnix supports (pg. 36). I *assume* that we need to write handlers for each of these traps:

* `TRAP_KERNEL`
* `TRAP_CLOCK`
* `TRAP_ILLEGAL`
* `TRAP_MEMORY`
* `TRAP_MATH`
* `TRAP_TTY_RECEIVE`
* `TRAP_TTY_TRANSMIT`
* `TRAP_DISK`
