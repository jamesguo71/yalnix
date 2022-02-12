# Yalnix - Team Unics
This repository contains our (Fei & Taylor) code for the COSC258 Yalnix project.

# Checkpoint 3

* Create a userland process `init.c` that simply loops forever traceprinting and pausing (i.e., same as DoIdle)
	* Setup a PCB for `initPCB` with a region 1 page table, frames for its kernel stack, a `UserContext` (the one from `_uctxt` in `KernelStart`), and a pid from `helper_new_pid()`. NOTE: We will have to update `KernelStart` to build the `initPCB` and we want to run this *first* instead of `DoIdle`.

* Your kernel should switch between `init` and `idle` on every clock trap

* Your kernel should also implement `Brk`, `GetPid`, and `Delay`.

* Your kernel should implement `KCCopy()`
	* Use this function to copy the current `KernelContext` into initPCB. TODO: Do we do this in `KernelStart` and/or `TrapClock`?
	* copy the contents of the current kernel stack into the new kernel stack frames

* Your kernel will need to implement `LoadProgram.c` such that it opens the `init` executable and sets it up in Region 1 memory
	* This function loads the executable into the *current* Region 1, so make sure you have already created the R1 page table and set that as the current PTBR1.

* Update `KernelStart` to check `cmd_args[0]`. If initialized, this will contain the name of the init program. Pass the whole `cmd_args` array as arguments for the new process.

* Update `TrapClock` to switch between the `DoIdle` and `init` processes