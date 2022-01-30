#ifndef __KERNEL_H
#define __KERNEL_H

int SetKernelBrk(void *_brk);
void KernelStart (char **cmd_args, unsigned int pmem_size, UserContext *uctxt);
KernelContext *MyKCS(KernelContext *, void *, void *);
KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used);

#endif