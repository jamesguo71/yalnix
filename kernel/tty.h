#ifndef __TTY_H
#define __TTY_H
#include <hardware.h>

#define TTY_NUM_TERMINALS NUM_TERMINALS

typedef struct tty_list tty_list_t;


/*!
 * \desc    Initializes memory for a new tty_list_t struct
 *
 * \return  An initialized tty_list_t struct, NULL otherwise.
 */
tty_list_t *TTYCreate();


/*!
 * \desc            Frees the memory associated with a tty_list_t struct
 *
 * \param[in] _tl  A tty_list_t struct that the caller wishes to free
 */
int  TTYDelete(tty_list_t *_tl);
int  TTYRead(tty_list_t *_tl, UserContext *_uctxt, int _tty_id, void *_usr_write_buf, int _buf_len);
int  TTYWrite(tty_list_t *_tl, UserContext *_uctxt, int _tty_id, void *_buf, int _len);
void TTYUpdateWriter(tty_list_t *_tl, UserContext *_uctxt, int _tty_id);
int  TTYUpdateReader(tty_list_t *_tl, int _tty_id);
#endif // __TTY_H