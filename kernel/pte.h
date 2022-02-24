#ifndef __PTE_H
#define __PTE_H
#include <hardware.h>

int  PTEAddressToPage(void *_address);
int  PTECheckAddress(pte_t *_pt, void *_address, int _length, int _prot);
void PTEClear(pte_t *_pt, int _page_num);
void PTESet(pte_t *_pt, int _page_num, int _prot, int _pfn);
void PTEPrint(pte_t *_pt);
#endif // __PTE_H