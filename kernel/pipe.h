#ifndef __PIPE_H
#define __PIPE_H
#include <hardware.h>

typedef struct pipe_list pipe_list_t;


/*!
 * \desc    Initializes memory for a new pipe_list_t struct
 *
 * \return  An initialized pipe_list_t struct, NULL otherwise.
 */
pipe_list_t *PipeListCreate();


/*!
 * \desc                  Frees the memory associated with a pipe_list_t struct
 *
 * \param[in] _pl  A pipe_list_t struct that the caller wishes to free
 */
int PipeListDelete(pipe_list_t *_pl);
int PipeInit(pipe_list_t *_pl, int *_pipe_id);
int PipeRead(pipe_list_t *_pl, UserContext *_uctxt, int _pipe_id, void *_buf, int _buf_len);
int PipeWrite(pipe_list_t *_pl, UserContext *_uctxt, int _pipe_id, void *_buf, int _buf_len);
#endif // __PIPE_H