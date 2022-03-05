#ifndef YALNIX_FRAMEWORK_YALNIX_KERNEL_BITVEC_H_
#define YALNIX_FRAMEWORK_YALNIX_KERNEL_BITVEC_H_

int PipeIDFindAndSet();

void PipeIDRetire(int i) ;

int PipeIDIsValid(int i) ;

int LockIDFindAndSet() ;

void LockIDRetire(int i) ;

int LockIDIsValid(int i) ;

int CVarIDFindAndSet() ;

void CVarIDRetire(int i) ;

int CVarIDIsValid(int i) ;

#endif //YALNIX_FRAMEWORK_YALNIX_KERNEL_BITVEC_H_
