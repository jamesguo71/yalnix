#include <hardware.h>
#include <ykernel.h>

#include "hardware.h"
#include "pte.h"
#include "kernel.h"


int PTEAddressToPage(void *_address) {
    // 1.
    if (!_address) {
        TracePrintf(1, "[PTEAddressToPage] Invalid address pointer\n");
        return ERROR;
    }
    return ((int ) _address) >> PAGESHIFT;
}


int PTECheckAddress(pte_t *_pt, void *_address, int _length, int _prot) {
    // 1. Check that our page table, address, and length variables are valid values.
    if (!_pt || !_address || _length < 1) {
        TracePrintf(1, "[PTECheckAddress] One or more invalid arguments\n");
        return ERROR;
    }

    // 2. Calculate the page number of the incoming address; remember to subtract MAX_PT_LEN since
    //    region 1 addresses map to pages 128-255 but our page tables are indexed 0-127. 
    int start_page = PTEAddressToPage(_address) - MAX_PT_LEN;
    if (start_page < 0) {
        TracePrintf(1, "[PTECheckAddress] Invalid address: %p. Points below region 1\n", _address);
        return ERROR;
    }

    // 3. Calculate the number of pages that the buffer spans. For the start page, we had to check
    //    that it was not below region 1, but for the end page we need to make sure that it is not
    //    above region 1.
    int end_page = PTEAddressToPage(_address + _length) - MAX_PT_LEN;
    if (end_page >= MAX_PT_LEN) {
        TracePrintf(1, "[PTECheckAddress] Invalid address: %p. Points above region 1\n",
                                          _address + _length);
        return ERROR;
    }

    // 4. Calculate the number of pages that the buffer spans and check that each one is valid
    //    and has the proper protection bits. If not, return ERROR. Note that in the case that
    //    the buffer is on a single page our num_pages will be 0; thus, let i be <= num_pages.
    int num_pages = end_page - start_page;
    for (int i = 0; i <= num_pages; i++) {
        if (!_pt[start_page + i].valid) {
            TracePrintf(1, "[PTECheckAddress] Invalid address: %p. Page: %d not valid\n:",
                                              _address, start_page + i);
            return ERROR;
        }
        if (_pt[start_page + i].prot != _prot) {
            TracePrintf(1, "[PTECheckAddress] Invalid address: %p. Page: %d prot doesn't match\n:",
                                              _address, start_page + i);
            return ERROR;            
        }
    }
    return 0;
}


/*!
 * \desc             A
 * 
 * \param[in] _pt     T
 * \param[in] index  T
 */
void PTEClear(pte_t *_pt, int _page_num) {
    // 1. Check that our page table pointer is invalid. If not, print message and return error.
    if (!_pt) {
        TracePrintf(1, "[PTEClear] Invalid page table pointer\n");
        Halt();
    }

    // 2. Check that our page number is valid. If not, print message and return error.
    if (_page_num < 0 || _page_num >= MAX_PT_LEN) {
        TracePrintf(1, "[PTEClear] Invalid page number: %d\n", _page_num);
        Halt();
    }

    // 3. Check to see if the entry indicated by _page_num is already invalid.
    //    If so, print a warning message, but do not return ERROR. 
    if (!_pt[_page_num].valid) {
        TracePrintf(1, "[PTEClear] Warning: page %d is alread invalid\n", _page_num);
        Halt();
    }

    // 4. Mark the page table entry indicated by _page_num as invalid.
    _pt[_page_num].valid = 0;
    _pt[_page_num].prot  = 0;
    _pt[_page_num].pfn   = 0;
}

void PTEPrint(pte_t *_pt) {
    // 1. Check that our page table pointer is invalid. If not, print message and return error.
    if (!_pt) {
        TracePrintf(1, "[PTEClear] Invalid page table pointer\n");
        Halt();
    }

    for (int i = 0; i < MAX_PT_LEN; i++) {
        if (_pt[i].valid) {
            TracePrintf(1, "[PTEPrint] Page: %d valid: %d prot: %d pfn: %d\n",
                                       i, _pt[i].valid, _pt[i].prot, _pt[i].pfn);
        } else {
            TracePrintf(1, "[PTEPrint] Page: %d valid: %d prot: %d pfn: none\n",
                                       i, _pt[i].valid, _pt[i].prot);
        }
    }
}



/*!
 * \desc                A
 * 
 * \param[in] _pt        T
 * \param[in] _page_num  T
 * \param[in] prot      T
 * \param[in] pfn       T
 */
void PTESet(pte_t *_pt, int _page_num, int _prot, int _pfn) {
    // 1. Check that our page table pointer is invalid. If not, print message and return error.
    if (!_pt) {
        TracePrintf(1, "[PTESet] Invalid page table pointer\n");
        Halt();
    }

    // 2. Check that our page number is valid. If not, print message and return error.
    if (_page_num < 0 || _page_num >= MAX_PT_LEN) {
        TracePrintf(1, "[PTESet] Invalid page number: %d\n", _page_num);
        Halt();
    }

    // 3. Check that our frame number is valid. If not, print message and return error.
    if (_pfn < 0 || _pfn >= e_num_frames) {
        TracePrintf(1, "[PTESet] Invalid frame number: %d\n", _page_num);
        Halt();
    }

    // 4. Check to see if the entry indicated by _page_num is already valid.
    //    If so, print a warning message, but do not return ERROR. 
    if (_pt[_page_num].valid) {
        TracePrintf(1, "[PTESet] Warning: page %d is alread valid\n", _page_num);
        Halt();
    }

    // 5. Make the page table entry indicated by _page_num valid, set
    //    its protections and map it to the frame indicated by pfn.
    _pt[_page_num].valid = 1;
    _pt[_page_num].prot  = _prot;
    _pt[_page_num].pfn   = _pfn;
}