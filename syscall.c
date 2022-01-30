#include "syscall.h"

int internal_Fork (void) {
    // Create a new pcb for child process
    // Get a new pid for the child process
    // Copy user_context into the new pcb
    // Call KernelContextSwitch(KCCopy, *new_pcb, NULL) to copy the current process into the new pcb
    // 

}

int internal_Exec (char *, char **) {
    // 1. Check arguments. Return error if invalid.
}

void internal_Exit (int) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Wait (int internal_*) {
    // 1. Check arguments. Return error if invalid.
}

int internal_GetPid (void) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Brk (void *) {
    // 1. Check arguments. Return error if invalid.
}

int internal_Delay (int) {
    // 1. Check arguments. Return error if invalid.
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