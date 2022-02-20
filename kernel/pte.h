#ifndef __PTE_H
#define __PTE_H
#include <hardware.h>

void PTEClear(pte_t *_pt, int _page_num);
void PTESet(pte_t *_pt, int _page_num, int _prot, int _pfn);
#endif // __PTE_H