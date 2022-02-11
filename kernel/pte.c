#include <hardware.h>
#include <ykernel.h>

#include "pte.h"
#include "kernel.h"


/*!
 * \desc             A
 * 
 * \param[in] _pt     T
 * \param[in] index  T
 * 
 * \return  Frame index on success, ERROR otherwise.
 */
int PTEClear(pte_t *_pt, int _page_num) {
    // 1. Check that our page table pointer is invalid. If not, print message and return error.
    if (!_pt) {
        TracePrintf(1, "[PTEClear] Invalid page table pointer\n");
        return ERROR;
    }

    // 2. Check that our page number is valid. If not, print message and return error.
    if (_page_num < 0 || _page_num >= MAX_PT_LEN) {
        TracePrintf(1, "[PTEClear] Invalid page number: %d\n", _page_num);
        return ERROR;
    }

    // 3. Check to see if the entry indicated by _page_num is already invalid.
    //    If so, print a warning message, but do not return ERROR. 
    if (!_pt[_page_num].valid) {
        TracePrintf(1, "[PTEClear] Warning: page %d is alread invalid\n", _page_num);
    }

    // 4. Mark the page table entry indicated by _page_num as invalid.
    _pt[_page_num].valid = 0;
    _pt[_page_num].prot  = 0;
    _pt[_page_num].pfn   = 0;
    return 0;
}


/*!
 * \desc                A
 * 
 * \param[in] _pt        T
 * \param[in] _page_num  T
 * \param[in] prot      T
 * \param[in] pfn       T
 * 
 * \return  Frame index on success, ERROR otherwise.
 */
int PTESet(pte_t *_pt, int _page_num, int _prot, int _pfn) {
    // 1. Check that our page table pointer is invalid. If not, print message and return error.
    if (!_pt) {
        TracePrintf(1, "[PTESet] Invalid page table pointer\n");
        return ERROR;
    }

    // 2. Check that our page number is valid. If not, print message and return error.
    if (_page_num < 0 || _page_num >= MAX_PT_LEN) {
        TracePrintf(1, "[PTESet] Invalid page number: %d\n", _page_num);
        return ERROR;
    }

    // 3. Check that our frame number is valid. If not, print message and return error.
    if (_pfn < 0 || _pfn >= e_num_frames) {
        TracePrintf(1, "[PTESet] Invalid frame number: %d\n", _page_num);
        return ERROR;
    }

    // 4. Check to see if the entry indicated by _page_num is already valid.
    //    If so, print a warning message, but do not return ERROR. 
    if (_pt[_page_num].valid) {
        TracePrintf(1, "[PTESet] Warning: page %d is alread valid\n", _page_num);
    }

    // 5. Make the page table entry indicated by _page_num valid, set
    //    its protections and map it to the frame indicated by pfn.
    _pt[_page_num].valid = 1;
    _pt[_page_num].prot  = _prot;
    _pt[_page_num].pfn   = _pfn;
    return 0;
}