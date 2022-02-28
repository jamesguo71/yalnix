#ifndef __TTY_H
#define __TTY_H
#include <hardware.h>

#define TTY_NUM_TERMINALS NUM_TERMINALS

typedef struct tty tty_t;


/*!
 * \desc    Initializes memory for a new tty_t struct
 *
 * \return  An initialized tty_t struct, NULL otherwise.
 */
tty_t *TTYCreate();


/*!
 * \desc                  Frees the memory associated with a tty_t struct
 *
 * \param[in] _tty  A tty_t struct that the caller wishes to free
 */
int  TTYDelete(tty_t *_tty);
int  TTYRead(tty_t *_tty, UserContext *_uctxt, int _tty_id, void *_usr_write_buf, int _buf_len);
int  TTYWrite(tty_t *_tty, UserContext *_uctxt, int _tty_id, void *_buf, int _len);
void TTYUpdateWriter(tty_t *_tty, UserContext *_uctxt, int _tty_id);
int  TTYUpdateReader(tty_t *_tty, int _tty_id);
#endif // __TTY_H