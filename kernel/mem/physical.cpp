/// @file
/// @brief The kernel's physical memory management system.
///
/// The physical page management system is fairly simple - pages are marked as allocated or deallocated in a bitmap.
/// Requests for pages are satisfied from that. Note that pages that are free are marked with a 1 in the bitmap, not a
/// 0.
///
/// Inevitably, this simple approach will lead to issues with fragmentation if callers always require contiguous blocks
/// of pages. This is left for another day.

//#define ENABLE_TRACING

#include <string.h>

#include "mem.h"
#include "mem-int.h"

namespace
{
  // In the page allocation bitmap, a 1 indicates that the page is FREE.
  const uint64_t SIZE_OF_PAGE = 2097152;
  const uint64_t BITMAP_SIZE = MEM_MAX_SUPPORTED_PAGES / 64;

  // Determines whether or not a page has been allocated. A 1 indicates free, 0 indicates allocated.
  uint64_t phys_pages_alloc_bitmap[BITMAP_SIZE];

  // Determines whether or not a page actually exists to be used. A 1 indicates exists, 0 indicates invalid.
  uint64_t phys_pages_exist_bitmap[BITMAP_SIZE];

  // A simple count of the number of free pages.
  uint64_t free_pages;

  // Protects the bitmap from multi-threaded accesses.
  ipc::raw_spinlock bitmap_lock;
}

/// @brief Initialise the physical memory management subsystem.
///
/// **This function must only be called once**
///
/// @param e820_ptr Pointer to the system's e820 table.
void mem_init_gen_phys_sys(e820_pointer *e820_ptr)
{
  KL_TRC_ENTRY;

  uint64_t mask;

  ASSERT((e820_ptr != nullptr) && (e820_ptr->table_ptr != nullptr));

  // Fill in the free pages bitmap appropriately.
  mem_gen_phys_pages_bitmap(e820_ptr, phys_pages_alloc_bitmap, MEM_MAX_SUPPORTED_PAGES);

  // The kernel will be loaded in to consecutive pages in the base of RAM, so mark as many as are needed as allocated.
  for (uint16_t i = 0; i < MEM_NUM_KERNEL_PAGES; i++)
  {
    mem_clear_bitmap_page_bit(i * MEM_PAGE_SIZE, true);
  }

  memcpy(phys_pages_exist_bitmap, phys_pages_alloc_bitmap, sizeof(phys_pages_alloc_bitmap));

  // Count up the number of free pages.
  for(int i = 0; i < BITMAP_SIZE; i++)
  {
    mask = 1;
    for (int j = 0; j < 64; j++)
    {
      ASSERT(mask != 0);
      if ((phys_pages_alloc_bitmap[i] & mask) != 0)
      {
        free_pages++;
      }
      mask <<= 1;
    }
  }

  ipc_raw_spinlock_init(bitmap_lock);

  ASSERT(free_pages > 0);

  KL_TRC_EXIT;
}

/// @brief Allocate a number of physical pages to the caller.
///
/// **NOTE** At present, only a single contiguous page can be allocated.
///
/// @param num_pages Must be equal to 1.
///
/// @return The address of a newly allocated physical page.
void *mem_allocate_physical_pages(uint32_t num_pages)
{
  KL_TRC_ENTRY;

  uint64_t mask;
  uint64_t addr;

  // For the time being, only allow the allocation of single pages.
  ASSERT(num_pages == 1);
  ASSERT(free_pages > 0);

  // Spin through the list, looking for a free page. Upon finding one, mark it
  // as in use and return the relevant address.
  ipc_raw_spinlock_lock(bitmap_lock);
  for (uint64_t i = 0; i < MEM_MAX_SUPPORTED_PAGES / 64; i++)
  {
    mask = 0x8000000000000000;
    ASSERT(mask == 0x8000000000000000);
    for (uint64_t j = 0; j < 64; j++)
    {
      ASSERT(mask != 0);
      if ((phys_pages_alloc_bitmap[i] & mask) != 0)
      {
        addr = SIZE_OF_PAGE * ((64 * i) + j);
        ASSERT((phys_pages_exist_bitmap[i] & mask) != 0);
        phys_pages_alloc_bitmap[i] = phys_pages_alloc_bitmap[i] & (~mask);
        free_pages--;

        KL_TRC_TRACE(TRC_LVL::EXTRA, "Address found\n");
        KL_TRC_EXIT;
        KL_TRC_TRACE(TRC_LVL::FLOW, "Free pages -: ", free_pages, "\n");
        ipc_raw_spinlock_unlock(bitmap_lock);
        return (void *)addr;
      }
      mask = mask >> 1;
    }
  }

  ipc_raw_spinlock_unlock(bitmap_lock);
  KL_TRC_EXIT;

  panic("No free pages to allocate.");
  return nullptr;
}

/// @brief Deallocate a physical page, for use by someone else later.
///
/// Simply clear the relevant flag in the free pages bitmap.
///
/// @param start The address of the start of the physical page to deallocate.
///
/// @param num_pages Must be equal to 1.
void mem_deallocate_physical_pages(void *start, uint32_t num_pages)
{
  KL_TRC_ENTRY;

  uint64_t start_num = (uint64_t)start;

  ipc_raw_spinlock_lock(bitmap_lock);
  ASSERT(num_pages == 1);
  ASSERT(start_num % SIZE_OF_PAGE == 0);
  ASSERT(!mem_is_bitmap_page_bit_set(start_num));
  mem_set_bitmap_page_bit(start_num, false);
  free_pages++;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Free pages +: ", free_pages, "\n");
  ipc_raw_spinlock_unlock(bitmap_lock);

  KL_TRC_EXIT;
}

/// @brief Mark the page as free in the bitmap.
///
/// Note that no checking is done to ensure the page is within the physical pages available to the system.
///
/// @param page_addr The address of the physical page to mark free.
///
/// @param ignore_checks If set to false, the system will check that this page is known to exist before changing the
///                      state of the allocation bitmap. However, this interferes with generating the map to start with
///                      so ignore_checks can be set to true to bypass this.
void mem_set_bitmap_page_bit(uint64_t page_addr, const bool ignore_checks)
{
  KL_TRC_ENTRY;

  uint64_t bitmap_qword;
  uint64_t bitmap_idx;
  uint64_t mask;

  ASSERT((page_addr / SIZE_OF_PAGE) < MEM_MAX_SUPPORTED_PAGES);

  bitmap_idx = (page_addr / SIZE_OF_PAGE) % 64;
  bitmap_qword = (page_addr / SIZE_OF_PAGE) / 64;

  mask = 0x8000000000000000;
  mask >>= bitmap_idx;

  ASSERT(mask != 0);
  ASSERT(bitmap_qword < BITMAP_SIZE);
  ASSERT((ignore_checks) || ((phys_pages_exist_bitmap[bitmap_qword] & mask) != 0));

  phys_pages_alloc_bitmap[bitmap_qword] = phys_pages_alloc_bitmap[bitmap_qword] | mask;

  ASSERT(phys_pages_alloc_bitmap[bitmap_qword] != 0);

  KL_TRC_EXIT;
}

/// @brief Mark the page as in use in the bitmap.
///
/// Note that no checking is done to ensure the page is within the physical pages available to the system.
///
/// @param page_addr The address of the physical page to mark as in use.
///
/// @param ignore_checks If set to true, the system will not check that the bit being cleared is set to 1 before
///                      clearing it. If false, and the bit is not set then the system assumes a failure and panics.
void mem_clear_bitmap_page_bit(uint64_t page_addr, bool ignore_checks)
{
  KL_TRC_ENTRY;

  uint64_t bitmap_qword;
  uint64_t bitmap_idx;
  uint64_t mask;

  ASSERT((page_addr / SIZE_OF_PAGE) < MEM_MAX_SUPPORTED_PAGES);

  bitmap_idx = (page_addr / SIZE_OF_PAGE) % 64;
  bitmap_qword = (page_addr / SIZE_OF_PAGE) / 64;

  mask = 0x8000000000000000;
  mask >>= bitmap_idx;

  ASSERT(mask != 0);
  ASSERT(bitmap_qword < BITMAP_SIZE);

  ASSERT(ignore_checks || ((phys_pages_exist_bitmap[bitmap_qword] & mask) != 0));
  phys_pages_alloc_bitmap[bitmap_qword] = phys_pages_alloc_bitmap[bitmap_qword] & (~mask);

  KL_TRC_EXIT;
}

/// @brief Determine whether a specific page has its bit set in the pages bitmap.
///
/// Note that a true return value indicates the page is FREE.
///
/// @param page_addr The page address to check
///
/// @return TRUE implies the page is free (the bit is set), FALSE implies in use.
bool mem_is_bitmap_page_bit_set(uint64_t page_addr)
{
  KL_TRC_ENTRY;

  uint64_t bitmap_qword;
  uint64_t bitmap_idx;
  uint64_t mask;

  ASSERT((page_addr / SIZE_OF_PAGE) < MEM_MAX_SUPPORTED_PAGES);

  bitmap_idx = (page_addr / SIZE_OF_PAGE) % 64;
  bitmap_qword = (page_addr / SIZE_OF_PAGE) / 64;

  mask = 0x8000000000000000;
  mask >>= bitmap_idx;

  ASSERT(mask != 0);
  ASSERT(bitmap_qword < BITMAP_SIZE);

  KL_TRC_EXIT;

  return (phys_pages_alloc_bitmap[bitmap_qword] & mask);

}
