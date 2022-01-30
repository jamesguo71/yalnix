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
pcb_t *current;
pcb_t *ready_queue;
pcb_t *blocked;

```

#### Struct Name Here
*struct description here*

```c
// Struct code here
```

Taylor
======
KernelSetBrk
KCCopy
Exec
TtyRead
TtyWrite
PipeInit
PipeRead
PipeWrite
LockInit
Acquire
Release
CvarInit
CvarSignal

Fei
===
KernelStart
MyKCS
Fork
Exec
Exit
Wait
GetPid
Brk
Delay
CvarBroadcast
CvarWait
Reclaim