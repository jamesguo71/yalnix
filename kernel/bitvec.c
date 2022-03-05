#include <hardware.h>
#include <ykernel.h>
#include "bitvec.h"


#define INT_SIZE sizeof(int)
#define MAX_NUM_RES  (INT_SIZE * 256)

int pipe_bitvec[MAX_NUM_RES / INT_SIZE];
int lock_bitvec[MAX_NUM_RES / INT_SIZE];
int cvar_bitvec[MAX_NUM_RES / INT_SIZE];

void  SetBit( int A[ ],  int k )
{
    A[k/INT_SIZE] |= 1 << (k%INT_SIZE);      // Set the bit at the k-th position
}

void  ClearBit( int A[ ],  int k )
{
    A[k/INT_SIZE] &= ~(1 << (k%INT_SIZE)) ;  // RESET the bit at the k-th position
}

int TestBit( const int A[ ],  int k )
{
    return ( (A[k/INT_SIZE] & (1 << (k%INT_SIZE) )) != 0 ) ;  // Return TRUE if bit set
}

// Find a valid spot in the given bitvec, if failed, return ERROR
int FindAndSet(int *bitvec) {
    for (int i = 0; i < MAX_NUM_RES; i++) {
        if (TestBit(bitvec, i)) {
            SetBit(bitvec, i);
            return i;
        }
    }
    TracePrintf(1, "[BitVecFindAndSet] Failed to find a valid spot\n");
    return ERROR;
}

void Clear(int *bitvec, int i) {
    if (!TestBit(bitvec, i)) {
        TracePrintf(1, "[BitVecClear] Position %d already cleared!\n", i);
        Halt();
    }
    ClearBit(bitvec, i);
}

int PipeIDFindAndSet() {
    return FindAndSet(pipe_bitvec);
}

void PipeIDRetire(int i) {
    Clear(pipe_bitvec, i);
}

int PipeIDIsValid(int i) {
    return TestBit(pipe_bitvec, i);
}

int LockIDFindAndSet() {
    return FindAndSet(lock_bitvec);
}

void LockIDRetire(int i) {
    Clear(lock_bitvec, i);
}

int LockIDIsValid(int i) {
    return TestBit(lock_bitvec, i);
}

int CVarIDFindAndSet() {
    return FindAndSet(cvar_bitvec);
}

void CVarIDRetire(int i) {
    Clear(cvar_bitvec, i);
}

int CVarIDIsValid(int i) {
    return TestBit(cvar_bitvec, i);
}


