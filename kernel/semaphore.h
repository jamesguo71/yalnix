#ifndef YALNIX_FRAMEWORK_YALNIX_USER_SEMAPHORE_H_
#define YALNIX_FRAMEWORK_YALNIX_USER_SEMAPHORE_H_

#include "dllist.h"

extern dllist *e_sem_list;

int SemInit(int *sem_idp, int val);

int SemUp(UserContext *uctxt, int sem_id);

int SemDown(UserContext *uctxt, int sem_id);

int SemReclaim(int sem_id);

#endif //YALNIX_FRAMEWORK_YALNIX_USER_SEMAPHORE_H_
