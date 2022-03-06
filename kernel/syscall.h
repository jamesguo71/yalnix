#ifndef __SYSCALL_H
#define __SYSCALL_H


/*!
 * \desc    a
 * 
 * \return              0 on success, ERROR otherwise
 */
int SyscallFork (UserContext *_uctxt);


/*!
 * \desc                Replaces the currently running program in the calling process' memory with
 *                      the program stored in the file named by filename. The argumetn argvec
 *                      points to a vector of arguments to pass to the new program as its argument
 *                      list.
 * 
 * \param[in] filename  The file containing the new program to instantiate
 * \param[in] argvec    The address of the vector containing arguments to the new program
 * 
 * \return              0 on success, ERROR otherwise
 */
int SyscallExec (UserContext *_uctxt, char *_filename, char **_argvec);

void SyscallExit (UserContext *_uctxt, int);

int SyscallWait (UserContext *_uctxt, int *);

int SyscallGetPid (void);

int SyscallBrk (UserContext *_uctxt, void *_brk);

int SyscallDelay (UserContext *_uctxt, int _clock_ticks);

int SyscallReclaim(int id);

#endif
