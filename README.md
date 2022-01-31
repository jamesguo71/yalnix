# Yalnix - Team Unics
This repository contains our (Fei & Taylor) code for the COSC258 Yalnix project.


## Checkpoint 1: Pseudocode
The yalnix guide states that for our first checkpoint we need to (1) sketch all of our kernel data structures and (2) write pseudocode for all of our traps, syscalls, and major library functions. Below are the data structures and functions that I *think* we need to provide.

*TODO: Add a table of contents that links to each data structure and kernel function?*


### Kernel Structures
Below are descriptions and pseudocode for the structures that we think we will need to implement our Yalnix kernel.

#### Process Control Block (PCB)
*struct description here*

> Based on Page 71
```c
typedef struct pcb {
    struct pcb *parent; // For keeping track of process relationship
    pcb_t *children; 
    int pid;
    int brk;

    UserContext   *uctxt; // Defined in `hardware.h`    
    pte_t *ks_frames;   // Kernel Stack frames
    KernelContext *kctxt;    // Needed for KernelCopy? See Page 45 

    pte_t *ks_frames;
    pte_t *pt;              // Defined in `hardware.h`
    int exit_status;        // for saving the process's exit status, See Page 32
    int exited;             // if the process has exited?  

    int *held_locks;        // locks held by the current process, used by sync syscalls
} pcb_t;
```

#### Process scheduling

```c
// We probably need to implement a queue for each of the following? Array or Linked List?
pcb_t *current;
pcb_t *ready_queue;
pcb_t *blocked;
pcb_t *terminated; // Some use cases: program has illegal instructions (like accessing invalid pages);

// Questions:
// Do we consider `cvar` / `lock` to be part of the scheduling?
// Do we need to make the following separate? Probably.
// Also, do we need to group the waiting processes in a queue array? 
// So that we can index into the specific queue of the signaling cvar's waiting processes?
// internal_Reclaim(int id) accepts an id withouth specifying its resource type, do we need to keep a mapping from id to resource type?
// pcb_t *cvar_waiting; // For keeping track of the processes `cond_wait`ing;
// pcb_t *lock_waiting; // For keeping track of the processes waiting to `acquire`;
```

#### Lock

```c
typedef struct lock {
    int id;     
    int value;  // FREE or LOCKED?
    int owner; // which process currently owns the lock?
    int *waiting; // which processes are waiting for the lock?
} lock_t;
```

#### CVar

```c
typedef struct cvar {
    int id;
    int *waiting; // which processes are waiting for the cvar?
} cvar_t;
```

#### Counter of clock_ticks
We need a counter to keep track of the clock ticks after the system boot.
```
long cur_clock_ticks;
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
trap_clock(UserContext *context);
trap_illegal(UserContext *context);
int trap_memory(UserContext *context);
int trap_math(UserContext *context);