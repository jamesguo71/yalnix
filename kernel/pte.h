#ifndef __FRAME_H
#define __FRAME_H
#include <hardware.h>

int PTEClear(pte_t *pt, int page_num);
int PTESet(pte_t *pt, int page_num, int prot, int pfn);
#endif // __FRAME_H