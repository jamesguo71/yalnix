#ifndef __SYSCALL_H
#define __SYSCALL_H


/*!
 * \desc    a
 * 
 * \return  0 on success, specific error code (negative int) otherwise.
 */
int internal_Fork (void);


/*!
 * \desc              a
 * 
 * \param[in] a  a
 * 
 * \return            0 on success, specific error code (negative int) otherwise.
 */
int internal_Exec (char *, char **);

void internal_Exit (int);

int internal_Wait (int *);

int internal_GetPid (void);

int internal_Brk (void *);

int internal_Delay (int);

int internal_TtyRead (int, void *, int);

int internal_TtyWrite (int, void *, int);

int internal_PipeInit (int *);

int internal_PipeRead (int, void *, int);

int internal_PipeWrite (int, void *, int);

int internal_LockInit (int *);

int internal_Acquire (int);

int internal_Release (int);

int internal_CvarInit (int *);

int internal_CvarWait (int, int);

int internal_CvarSignal (int);

int internal_CvarBroadcast (int);

int internal_Reclaim (int);

#endif