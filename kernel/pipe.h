#ifndef __PIPE_H
#define __PIPE_H
#include <hardware.h>


typedef struct pipe_list pipe_list_t;

/*!
 * \desc    Initializes memory for a new pipe_list_t struct, which maintains a list of pipes.
 *
 * \return  An initialized pipe_list_t struct, NULL otherwise.
 */
pipe_list_t *PipeListCreate();


/*!
 * \desc           Frees the memory associated with a pipe_list_t struct
 *
 * \param[in] _pl  A pipe_list_t struct that the caller wishes to free
 */
int PipeListDelete(pipe_list_t *_pl);


/*!
 * \desc                 Creates a new pipe and saves the id at the caller specified address.
 *
 * \param[in]  _pl       An initialized pipe_list_t struct
 * \param[out] _pipe_id  The address where the newly created pipe's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int PipeInit(pipe_list_t *_pl, int *_pipe_id);


/*!
 * \desc                Removes the pipe from our pipe list and frees its memory, but *only* if no
 *                      other processes are currenting waiting on it. Otherwise, we return ERROR
 *                      and do not free the pipe.
 * 
 * \param[in] _pl       An initialized pipe_list_t struct
 * \param[in] _pipe_id  The id of the pipe that the caller wishes to free
 * 
 * \return              0 on success, ERROR otherwise
 */
int PipeReclaim(pipe_list_t *_pl, int _pipe_id);


/*!
 * \desc                 Reads from the pipe and stores the bytes in the caller's output buffer.
 *                       The caller is not guaranteed to get _buf_len bytes back, however, if
 *                       there are not enough bytes in the pipe---we simply return whatever is
 *                       in the pipe at the time. If there are no bytes or another process is
 *                       currently reading from the pipe, however, the caller is blocked until
 *                       the pipe is free and/or has bytes to read.
 * 
 * \param[in]  _pl       An initialized pipe_list_t struct
 * \param[in]  _uctxt    The UserContext for the current running process
 * \param[in]  _pipe_id  The id of the pipe that the caller wishes to read from
 * \param[out] _buf      The output buffer for storing the bytes read from the pipe
 * \param[in]  _buf_len  The length of the output buffer
 * 
 * \return               Number of bytes read on success, ERROR otherwise
 */
int PipeRead(pipe_list_t *_pl, UserContext *_uctxt, int _pipe_id, void *_buf, int _buf_len);


/*!
 * \desc                Writes the bytes from the caller's input buffer into the the pipe.
 *                      Unlike read, this function guarantees to write *all* bytes from the
 *                      input buffer into the pipe, though it may require blocking a number of
 *                      times if the input buffer is (1) larger than the available space in the
 *                      pipe or (2) if others are currently writing to the pipe.
 * 
 * \param[in] _pl       An initialized pipe_list_t struct
 * \param[in] _uctxt    The UserContext for the current running process
 * \param[in] _pipe_id  The id of the pipe that the caller wishes to write to
 * \param[in] _buf      The input buffer containing bytes to write to the pipe
 * \param[in] _buf_len  The length of the input buffer
 * 
 * \return               Number of bytes read on success, ERROR otherwise
 */
int PipeWrite(pipe_list_t *_pl, UserContext *_uctxt, int _pipe_id, void *_buf, int _buf_len);
#endif // __PIPE_H