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
// Struct code here
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
	// 1. Check arguments. Return error if invalid. The new brk should be a valid pointer
}
```

#### KernelStart
*function description here*

```c
void KernelStart (char**, unsigned int, UserContext *);
```

#### MyKCS
*function description here*

```c
KernelContext *MyKCS(KernelContext *, void *, void *)
```

#### KCCopy
*function description here*

```c
KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used);
```

#### Nop
*function description here*

```c
int Nop (int,int,int,int);
```

#### Fork
*function description here*

```c
int Fork (void);
```

#### Exec
*function description here*

```c
int Exec (char *, char **);
```

#### Exit
*function description here*

```c
void Exit (int);
```

#### Wait
*function description here*

```c
int Wait (int *);
```

#### GetPid
*function description here*

```c
int GetPid (void);
```

#### Brk
*function description here*

```c
int Brk (void *);
```

#### Delay
*function description here*

```c
int Delay (int);
```

#### TtyRead
*function description here*

```c
int TtyRead (int, void *, int);
```

#### TtyWrite
*function description here*

```c
int TtyWrite (int, void *, int);
```

#### Register
*function description here*

```c
int Register (unsigned int);
```

#### Send
*function description here*

```c
int Send (void *, int);
```

#### Receive
*function description here*

```c
int Receive (void *);
```

#### ReceiveSpecific
*function description here*

```c
int ReceiveSpecific (void *, int);
```

#### Reply
*function description here*

```c
int Reply (void *, int);
```

#### Forward
*function description here*

```c
int Forward (void *, int, int);
```

#### CopyFrom
*function description here*

```c
int CopyFrom (int, void *, void *, int);
```

#### CopyTo
*function description here*

```c
int CopyTo (int, void *, void *, int);
```

#### ReadSector
*function description here*

```c
int ReadSector (int, void *);
```

#### WriteSector
*function description here*

```c
int WriteSector (int, void *);
```

#### PipeInit
*function description here*

```c
int PipeInit (int *);
```

#### PipeRead
*function description here*

```c
int PipeRead (int, void *, int);
```

#### PipeWrite
*function description here*

```c
int PipeWrite (int, void *, int);
```

#### SemInit
*function description here*

```c
int SemInit (int *, int);
```

#### SemUp
*function description here*

```c
int SemUp (int);
```

#### SemDown
*function description here*

```c
int SemDown (int);
```

#### LockInit
*function description here*

```c
int LockInit (int *);
```

#### Acquire
*function description here*

```c
int Acquire (int);
```

#### Release
*function description here*

```c
int Release (int);
```

#### CvarInit
*function description here*

```c
int CvarInit (int *);
```

#### CvarWait
*function description here*

```c
int CvarWait (int, int);
```

#### CvarSignal
*function description here*

```c
int CvarSignal (int);
```

#### CvarBroadcast
*function description here*

```c
int CvarBroadcast (int);
```

#### Reclaim
*function description here*

```c
int Reclaim (int);
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
