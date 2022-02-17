#include "frame.h"
#include "kernel.h"
#include "ykernel.h"

//Bit Manipulation: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/Progs/bit-array2.c
#define BitClear(A,k) ( A[(k/KERNEL_BYTE_SIZE)] &= ~(1 << (k%KERNEL_BYTE_SIZE)) )
#define BitSet(A,k)   ( A[(k/KERNEL_BYTE_SIZE)] |=  (1 << (k%KERNEL_BYTE_SIZE)) )
#define BitTest(A,k)  ( A[(k/KERNEL_BYTE_SIZE)] &   (1 << (k%KERNEL_BYTE_SIZE)) )


/*!
 * \desc                 Marks the frame indicated by "_frame_num" as free by clearing its bit
 *                       in our global frame bit vector.
 * 
 * \param[in] _frame_num  The number of the frame to free
 * 
 * \return               0 on success, ERROR otherwise.
 */
int FrameClear(int _frame_num) {
    // 1. Check that our frame number is valid. If not, print message and return error.
    if (_frame_num < 0 || _frame_num >= e_num_frames) {
        TracePrintf(1, "[FrameClear] Invalid frame number: %d\n", _frame_num);
        return ERROR;
    }

    // 2. Check that our frames bit vector is initialized. If not, print message and halt.
    if (!e_frames) {
    	TracePrintf(1, "[FrameClear] Frame bit vector e_frames is not initialized\n");
    	Halt();
    }

    // 3. Check to see if the frame indicated by _frame_num is already invalid.
    //    If so, print a warning message but do not return ERROR.
    if(!BitTest(e_frames, _frame_num)) {
        TracePrintf(1, "[FrameClear] Warning: frame %d is already invalid\n", _frame_num);
    }

    // 4. "Free" the frame indicated by _frame_num by clearing its bit in our frames bit vector.
    BitClear(e_frames, _frame_num);
    return 0;
}


/*!
 * \desc    Find a free frame from the physical memory
 * 
 * \return  Frame number on success, ERROR otherwise.
 */
int FrameFind() {
    // 1. Check that our frames bit vector is initialized. If not, print message and halt.
    if (!e_frames) {
    	TracePrintf(1, "[FrameFind] Frame bit vector e_frames is not initialized\n");
    	Halt();
    }

    // 2. Loop over the frame bit vector and return the index of the first free frame
    //    we find. If we make it through the loop, there must not be any free frames.
    for (int i = 0; i < e_num_frames; i++) {
        if (BitTest(e_frames, i) == 0) {
            BitSet(e_frames, i);
            return i;
        }
    }
    return ERROR;
}


/*!
 * \desc                 Marks the frame indicated by "_frame_num" as in use by setting its bit
 *                       in our global frame bit vector.
 * 
 * \param[in] _frame_num  The number of the frame to mark as in use
 * 
 * \return               0 on success, ERROR otherwise.
 */
int FrameSet(int _frame_num) {
    // 1. Check that our frame number is valid. If not, print message and return error.
    if (_frame_num < 0 || _frame_num >= e_num_frames) {
        TracePrintf(1, "[FrameSet] Invalid frame number: %d\n", _frame_num);
        return ERROR;
    }

    // 2. Check that our frames bit vector is initialized. If not, print message and halt.
    if (!e_frames) {
    	TracePrintf(1, "[FrameSet] Frame bit vector e_frames is not initialized\n");
    	Halt();
    }

    // 3. Check to see if the frame indicated by _frame_num is already valid.
    //    If so, print a warning message but do not return ERROR.
    if(BitTest(e_frames, _frame_num)) {
        TracePrintf(1, "[FrameSet] Warning: frame %d is already valid\n", _frame_num);
    }

    // 4. Mark the frame indicated by _frame_num as in use by
    //    setting its bit in the frame bit vector.
    BitSet(e_frames, _frame_num);
    return 0;
}