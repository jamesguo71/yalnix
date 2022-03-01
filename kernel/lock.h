#ifndef __LOCK_H
#define __LOCK_H
#include <hardware.h>

typedef struct lock_list lock_list_t;


/*!
 * \desc    Initializes memory for a new lock_list_t struct
 *
 * \return  An initialized lock_list_t struct, NULL otherwise.
 */
lock_list_t *LockListCreate();

/*!
 * \desc                  Frees the memory associated with a lock_list_t struct
 *
 * \param[in] _ll  A lock_list_t struct that the caller wishes to free
 */
int LockListDelete(lock_list_t *_ll);

/*!
 * \desc                 Creates a new lock.
 * 
 * \param[out] lock_idp  The address where the newly created lock's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int LockInit (lock_list_t *_ll, int *lock_idp);

/*!
 * \desc               Acquires the lock identified by lock_id
 * 
 * \param[in] lock_id  The id of the lock to be acquired
 * 
 * \return             0 on success, ERROR otherwise
 */
int LockAcquire (lock_list_t *_ll, UserContext *_uctxt, int lock_id);

/*!
 * \desc               Releases the lock identified by lock_id
 * 
 * \param[in] lock_id  The id of the lock to be released
 * 
 * \return             0 on success, ERROR otherwise
 */
int LockRelease (lock_list_t *_ll, UserContext *_uctxt, int lock_id);

int LockReclaim(lock_list_t *_ll, int _lock_id);
#endif // __LOCK_H