#ifndef __CVAR_H
#define __CVAR_H
#include <hardware.h>

typedef struct cvar_list cvar_list_t;


/*!
 * \desc    Initializes memory for a new cvar_list_t struct
 *
 * \return  An initialized cvar_list_t struct, NULL otherwise.
 */
cvar_list_t *CVarListCreate();

/*!
 * \desc                  Frees the memory associated with a cvar_list_t struct
 *
 * \param[in] _cl  A cvar_list_t struct that the caller wishes to free
 */
int CVarListDelete(cvar_list_t *_cl);

/*!
 * \desc                 Creates a new cvar.
 * 
 * \param[out] cvar_idp  The address where the newly created cvar's id should be stored
 * 
 * \return               0 on success, ERROR otherwise
 */
int CVarInit(cvar_list_t *_cl, int *_cvar_id);

/*!
 * \desc               Acquires the cvar identified by cvar_id
 * 
 * \param[in] cvar_id  The id of the cvar to be acquired
 * 
 * \return             0 on success, ERROR otherwise
 */
int CVarSignal(cvar_list_t *_cl, int _cvar_id);

/*!
 * \desc               Releases the cvar identified by cvar_id
 * 
 * \param[in] cvar_id  The id of the cvar to be released
 * 
 * \return             0 on success, ERROR otherwise
 */
int CVarBroadcast(cvar_list_t *_cl, int _cvar_id);

/*!
 * \desc               Releases the cvar identified by cvar_id
 * 
 * \param[in] cvar_id  The id of the cvar to be released
 * 
 * \return             0 on success, ERROR otherwise
 */
int CVarWait(cvar_list_t *_cl, UserContext *_uctxt, int _cvar_id, int _lock_id);

int CVarReclaim(cvar_list_t *_cl, int _cvar_id);
#endif // __CVAR_H