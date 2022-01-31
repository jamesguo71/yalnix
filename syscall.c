#include "syscall.h"

int internal_Fork (void) {
    // Reference Checkpoint 3 for more details
    // Create a new pcb for child process
    // Get a new pid for the child process
    // Copy user_context into the new pcb
    // Call KernelContextSwitch(KCCopy, *new_pcb, NULL) to copy the current process into the new pcb
    // For each valid pte in the page table of the parent process, find a free frame and change the page table entry to map into this frame
    // Copy the frame from parent to child
    // Copy the frame stack to a new frame stack by temporarily mapping the destination frame into some page
    // E.g, the page right below the kernel stack.
    // Make the return value different between parent process and child process
}

int internal_Exec (char *filename, char **argvec) {
    // Should be pretty similar to what LoadProgram in `template.c` does
    // Open the file
    // Calculate the start and end position of each section (Text, Data, Stack)
    // Reserve space for argc and argv on the stack
    // Throw away the old page table and build up the new one
    // Flush TLB
    // Read the data from file into memory
    // Change the PROT field of the TEXT section
    // Set the entry point in the process's UserContext
    // Build the argument list on the new stack.
}

void internal_Exit (int status) {
    // Free all the resources in the process's pcb
    // Check to see if the current process has a parent (parent pointer is null?), if so, save the `status` parameter into the pcb
    // Otherwise, free this pcb in the kernel (?) or put it on an orphan list (?)
    // Check if the process is the initial process, if so, halt the system.
}

int internal_Wait (int *status_ptr) {
    // Check to see if the process's children list is empty (children is a null pointer), 
    // if so, return ERROR
    // Otherwise, walk through the process's children list and see if there's a process with `exited = 1`
    // If so, continue to next step; If not, block the current process until if its has a child with `exited=1`
    // Check if status_ptr is not null, collect its status and Save its exit status into `status_ptr`, otherwise just continue 
    // retire its pid, delete it from the process's children list, then return    
    // then, the call returns with the pid of that child.
}

int internal_GetPid (void) {
    // Return the pid of the current pcb
}

int internal_Brk (void *addr) {    
    // Check if addr is greater or less than the current brk of the process
    // If greater, check if it is below the Userland Red Zone
        // Return ERROR if not
        // Call UP_TO_PAGE to get the suitable new addr
        // Adjust page table to account for the new brk
    // If less, check if it is above the USER DATA limit
        // Return ERROR if not
        // Call DOWN_TO_PAGE to get the suitable new addr
        // Adjust page table to account for the new brk
    return 0
}

int internal_Delay (int clock_ticks) {
    // If clock ticks is 0, return is immediate. 
    // If clock ticks is less than 0, return ERROR
    // Block current process until clock_ticks clock interrupts have occurred after the call
    // Upon completion of the delay, the value 0 is returned
}

int internal_TtyRead (int, void *, int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_TtyWrite (int, void *, int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_PipeInit (int *) {
    // 1. Check arguments. Return error if invalid.
}

int internal_PipeRead (int, void *, int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_PipeWrite (int, void *, int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_LockInit (int *) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Acquire (int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Release (int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_CvarInit (int *) {
    // 1. Check arguments. Return error if invalid.
}

int internal_CvarWait (int, int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_CvarSignal (int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_CvarBroadcast (int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Reclaim (int) {
    // 1. Check arguments. Return error if invalid.
}