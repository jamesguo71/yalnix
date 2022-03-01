#ifndef __CVAR_H
#define __CVAR_H
#include <hardware.h>

#define CVAR_ID_START 0

typedef struct cvar_list cvar_list_t;


/*!
 * \desc    Initializes memory for a new cvar_list_t struct, which maintains a list of cvars.
 *
 * \return  An initialized cvar_list_t struct, NULL otherwise.
 */
cvar_list_t *CVarListCreate();


/*!
 * \desc           Frees the memory associated with a cvar_list_t struct
 *
 * \param[in] _cl  A cvar_list_t struct that the caller wishes to free
 */
int CVarListDelete(cvar_list_t *_cl);


/*!
 * \desc                 Creates a new cvar and saves the id at the caller specified address.
 *
 * \param[in]  _cl       An initialized cvar_list_t struct
 * \param[out] _cvar_id  The address where the newly created cvar's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int CVarInit(cvar_list_t *_cl, int *_cvar_id);


/*!
 * \desc                Unblocks the next process waiting on the cvar (if any)
 * 
 * \param[in] _cl       An initialized cvar_list_t struct
 * \param[in] _cvar_id  The id of the cvar that the caller wishes to signal on
 * 
 * \return              0 on success, ERROR otherwise
 */
int CVarSignal(cvar_list_t *_cl, int _cvar_id);


/*!
 * \desc                Unblocks *all* processes waiting on the cvar (if any)
 * 
 * \param[in] _cl       An initialized cvar_list_t struct
 * \param[in] _cvar_id  The id of the cvar that the caller wishes to signal on
 * 
 * \return              0 on success, ERROR otherwise
 */
int CVarBroadcast(cvar_list_t *_cl, int _cvar_id);


/*!
 * \desc                Releases the caller held lock and adds the caller to the cvar wait list.
 *                      When the caller is unblocked (due to a signal or broadcast) we re-aquire
 *                      the lock before returning. If the caller does not hold the lock at the
 *                      time this function is called we return immediately with ERROR.
 * 
 * \param[in] _cl       An initialized cvar_list_t struct
 * \param[in] _uctxt    The UserContext for the current running process
 * \param[in] _cvar_id  The id of the cvar that the caller wishes to wait on
 * \param[in] _lock_id  The id of the lock that the caller current holds
 * 
 * \return              0 on success, ERROR otherwise
 */
int CVarWait(cvar_list_t *_cl, UserContext *_uctxt, int _cvar_id, int _lock_id);


/*!
 * \desc                Removes the cvar from our cvar list and frees its memory, but *only* if no
 *                      other processes are currenting waiting on it. Otherwise, we return ERROR
 *                      and do not free the cvar.
 * 
 * \param[in] _cl       An initialized cvar_list_t struct
 * \param[in] _cvar_id  The id of the cvar that the caller wishes to free
 * 
 * \return              0 on success, ERROR otherwise
 */
int CVarReclaim(cvar_list_t *_cl, int _cvar_id);
#endif // __CVAR_H