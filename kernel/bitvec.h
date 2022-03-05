#ifndef YALNIX_FRAMEWORK_YALNIX_KERNEL_BITVEC_H_
#define YALNIX_FRAMEWORK_YALNIX_KERNEL_BITVEC_H_

#define INT_SIZE ((int) sizeof(int))
#define MAX_NUM_RES  (INT_SIZE * 256)

#define PIPE_BEGIN_INDEX 0
#define PIPE_LIMIT MAX_NUM_RES

#define LOCK_BEGIN_INDEX MAX_NUM_RES
#define LOCK_LIMIT (2 * MAX_NUM_RES)

#define CVAR_BEGIN_INDEX (2 * MAX_NUM_RES)
#define CVAR_LIMIT (3 * MAX_NUM_RES)

int PipeIDFindAndSet();

void PipeIDRetire(int pipe_id) ;

int PipeIDIsValid(int pipe_id) ;

int LockIDFindAndSet() ;

void LockIDRetire(int lock_id) ;

int LockIDIsValid(int lock_id) ;

int CVarIDFindAndSet() ;

void CVarIDRetire(int cvar_id) ;

int CVarIDIsValid(int cvar_id) ;

#endif //YALNIX_FRAMEWORK_YALNIX_KERNEL_BITVEC_H_
