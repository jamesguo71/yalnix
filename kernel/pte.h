#ifndef __PTE_H
#define __PTE_H
#include <hardware.h>

int PTEClear(pte_t *pt, int page_num);
int PTESet(pte_t *pt, int page_num, int prot, int pfn);
#endif // __PTE_H