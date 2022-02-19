#ifndef __SYSCALL_H
#define __SYSCALL_H


/*!
 * \desc    a
 * 
 * \return              0 on success, ERROR otherwise
 */
int SyscallFork (void);


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
int SyscallExec (char *filename, char **argvec);

void SyscallExit (UserContext *_uctxt, int);

int SyscallWait (UserContext *_uctxt, int *);

int SyscallGetPid (void);

int SyscallBrk (void *);

int SyscallDelay (UserContext *_uctxt, int _clock_ticks);


/*!
 * \desc               Reads the next line of input from the terminal indicated by tty_id.
 * 
 * \param[in]  tty_id  The id of the terminal to read from
 * \param[out] buf     An output buffer to store the bytes read from the terminal
 * \param[in]  len     The length of the output buffer
 * 
 * \return             Number of bytes read on success, ERROR otherwise
 */
int SyscallTtyRead (UserContext *_uctxt, int tty_id, void *buf, int len);


/*!
 * \desc              Writes the contents of the buffer to the terminal indicated by tty_id.
 * 
 * \param[in] tty_id  The id of the terminal to write to
 * \param[in] buf     An input buffer containing the bytes to write to the terminal
 * \param[in] len     The length of the input buffer
 * 
 * \return            Number of bytes written on success, ERROR otherwise
 */
int SyscallTtyWrite (UserContext *_uctxt, int tty_id, void *buf, int len);


/*!
 * \desc                 Creates a new pipe.
 * 
 * \param[out] pipe_idp  The address where the newly created pipe's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int SyscallPipeInit (int *pipe_idp);


/*!
 * \desc                Reads len number of bytes from the pipe indicated by pipe_id into buf.
 * 
 * \param[in]  pipe_id  The id of the pipe to read from
 * \param[out] buf      An output buffer to store the bytes read from the pipe
 * \param[in]  len      The length of the output buffer
 * 
 * \return              Number of bytes read on success, ERROR otherwise
 */
int SyscallPipeRead (int pipe_id, void *buf, int len);


/*!
 * \desc               Writes the contents of the buffer to the pipe indicated by pipe_id.
 * 
 * \param[in] pipe_id  The id of the terminal to write to
 * \param[in] buf      An input buffer containing the bytes to write to the pipe
 * \param[in] len      The length of the input buffer
 * 
 * \return             Number of bytes written on success, ERROR otherwise
 */
int SyscallPipeWrite (int pipe_id, void *buf, int len);


/*!
 * \desc                 Creates a new lock.
 * 
 * \param[out] lock_idp  The address where the newly created lock's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int SyscallLockInit (int *lock_idp);


/*!
 * \desc               Acquires the lock identified by lock_id
 * 
 * \param[in] lock_id  The id of the lock to be acquired
 * 
 * \return             0 on success, ERROR otherwise
 */
int SyscallAcquire (int lock_id);


/*!
 * \desc               Releases the lock identified by lock_id
 * 
 * \param[in] lock_id  The id of the lock to be released
 * 
 * \return             0 on success, ERROR otherwise
 */
int SyscallRelease (int lock_id);


/*!
 * \desc                 Creates a new condition variable.
 * 
 * \param[out] cvar_idp  The address where the newly created cvar's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int SyscallCvarInit (int *cvar_idp);


/*!
 * \desc               Signal the condition variable identified by cvar_id
 * 
 * \param[in] cvar_id  The id of the cvar to be signaled
 * 
 * \return             0 on success, ERROR otherwise
 */
int SyscallCvarSignal (int cvar_id);

int SyscallCvarBroadcast (int);

int SyscallCvarWait (int, int);

int SyscallReclaim (int);

#endif
