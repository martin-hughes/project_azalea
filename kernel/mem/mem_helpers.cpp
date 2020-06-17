/// @file
/// @brief Functions that are useful when manipulating memory addresses

//#define ENABLE_TRACING

#include <stdint.h>

#include "mem.h"

/// @brief Split a memory address into page address and offset.
///
/// This function splits a memory address to retrieve the address of the start of the page that address is within, as
/// well as the offset of that address within its page.
///
/// @param[in] base_addr The address to split.
///
/// @param[out] page_addr The address of the start of the page containing base_addr.
///
/// @param[out] page_offset The offset of base_addr within the page starting at page_addr.
void klib_mem_split_addr(uint64_t base_addr, uint64_t &page_addr, uint64_t &page_offset)
{
  KL_TRC_ENTRY;

  page_offset = base_addr % MEM_PAGE_SIZE;
  page_addr = base_addr - page_offset;

  KL_TRC_EXIT;
}
