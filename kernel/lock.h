#ifndef __LOCK_H
#define __LOCK_H
#include <hardware.h>

#define LOCK_ID_START 1000000

typedef struct lock_list lock_list_t;


/*!
 * \desc    Initializes memory for a new lock_list_t struct, which maintains a list of locks.
 *
 * \return  An initialized lock_list_t struct, NULL otherwise.
 */
lock_list_t *LockListCreate();


/*!
 * \desc           Frees the memory associated with a lock_list_t struct
 *
 * \param[in] _ll  A lock_list_t struct that the caller wishes to free
 */
int LockListDelete(lock_list_t *_ll);


/*!
 * \desc                 Creates a new lock and saves the id at the caller specified address.
 *
 * \param[in]  _ll       An initialized lock_list_t struct
 * \param[out] _lock_id  The address where the newly created lock's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int LockInit(lock_list_t *_ll, int *_lock_id, int check_addr_flag);


/*!
 * \desc                Acquires the lock for the caller. If the lock is currently taken, it will
 *                      block the caller and will not run again until a call to LockRelease moves
 *                      it back to the ready queue.
 * 
 * \param[in] _ll       An initialized lock_list_t struct
 * \param[in] _uctxt    The UserContext for the current running process
 * \param[in] _lock_id  The id of the lock that the caller wishes to acquire
 * 
 * \return              0 on success, ERROR otherwise
 */
int LockAcquire(lock_list_t *_ll, UserContext *_uctxt, int _lock_id);


/*!
 * \desc                Releases the lock, but only if held by the caller. If not, ERROR is
 *                      returned and the lock is not released.
 * 
 * \param[in] _ll       An initialized lock_list_t struct
 * \param[in] _lock_id  The id of the lock that the caller wishes to release
 * 
 * \return              0 on success, ERROR otherwise
 */
int LockRelease(lock_list_t *_ll, int _lock_id);


/*!
 * \desc                Removes the lock from our lock list and frees its memory, but *only* if no
 *                      other processes are currenting waiting on it. Otherwise, we return ERROR
 *                      and do not free the lock.
 * 
 * \param[in] _ll       An initialized lock_list_t struct
 * \param[in] _lock_id  The id of the lock that the caller wishes to free
 * 
 * \return              0 on success, ERROR otherwise
 */
int LockReclaim(lock_list_t *_ll, int _lock_id);
#endif // __LOCK_H