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
*function description here*

```c
void KernelStart (char**, unsigned int, UserContext *) {
	// 1. Check arguments. Return error if invalid.
}
```

#### MyKCS
*function description here*

```c
KernelContext *MyKCS(KernelContext *, void *, void *)
```

#### KCCopy
*function description here*

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
*function description here*

```c
int Fork (void) {
	// 1. Check arguments. Return error if invalid.
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
