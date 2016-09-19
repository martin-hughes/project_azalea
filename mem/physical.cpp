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

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/mem-int.h"

// In the page allocation bitmap, a 1 indicates that the page is FREE.
const unsigned long MAX_SUPPORTED_PAGES = 2048;
const unsigned long SIZE_OF_PAGE = 2097152;
const unsigned long BITMAP_SIZE = MAX_SUPPORTED_PAGES / 64;
static unsigned long phys_pages_bitmap[BITMAP_SIZE];
static unsigned long free_pages;
static kernel_spinlock bitmap_lock;

/// @brief Initialise the physical memory management subsystem.
///
/// **This function must only be called once**
void mem_init_gen_phys_sys()
{
  KL_TRC_ENTRY;

  unsigned long mask;

  // Fill in the free pages bitmap appropriately.
  mem_gen_phys_pages_bitmap(phys_pages_bitmap, MAX_SUPPORTED_PAGES);

  // Count up the number of free pages.
  for(int i = 0; i < BITMAP_SIZE; i++)
  {
    mask = 1;
    for (int j = 0; j < 64; j++)
    {
      ASSERT(mask != 0);
      if ((phys_pages_bitmap[i] & mask) != 0)
      {
        free_pages++;
      }
      mask <<= 1;
    }
  }

  klib_synch_spinlock_init(bitmap_lock);

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
void *mem_allocate_physical_pages(unsigned int num_pages)
{
  KL_TRC_ENTRY;

  unsigned long mask;
  unsigned long addr;

  // For the time being, only allow the allocation of single pages.
  ASSERT(num_pages == 1);

  // Spin through the list, looking for a free page. Upon finding one, mark it
  // as in use and return the relevant address.
  klib_synch_spinlock_lock(bitmap_lock);
  for (int i = 0; i < MAX_SUPPORTED_PAGES / 64; i++)
  {
    mask = 0x8000000000000000;
    ASSERT(mask == 0x8000000000000000);
    for (int j = 0; j < 64; j++)
    {
      ASSERT(mask != 0);
      if ((phys_pages_bitmap[i] & mask) != 0)
      {
        addr = SIZE_OF_PAGE * ((64 * i) + j);
        phys_pages_bitmap[i] = phys_pages_bitmap[i] & (~mask);

        KL_TRC_TRACE(TRC_LVL::EXTRA, "Address found\n");
        KL_TRC_EXIT;
        klib_synch_spinlock_unlock(bitmap_lock);
        return (void *)addr;
      }
      mask = mask >> 1;
    }
  }

  klib_synch_spinlock_unlock(bitmap_lock);
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
void mem_deallocate_physical_pages(void *start, unsigned int num_pages)
{
  KL_TRC_ENTRY;

  unsigned long start_num = (unsigned long)start;

  ASSERT(num_pages == 1);
  ASSERT(start_num % SIZE_OF_PAGE == 0);
  ASSERT(!mem_is_bitmap_page_bit_set(start_num));
  mem_set_bitmap_page_bit(start_num);

  KL_TRC_EXIT;
}

/// @brief Mark the page as free in the bitmap.
///
/// Note that no checking is done to ensure the page is within the physical pages available to the system.
///
/// @param page_addr The address of the physical page to mark free.
void mem_set_bitmap_page_bit(unsigned long page_addr)
{
  KL_TRC_ENTRY;

  unsigned long bitmap_qword;
  unsigned long bitmap_idx;
  unsigned long mask;

  ASSERT((page_addr / SIZE_OF_PAGE) < MAX_SUPPORTED_PAGES);

  bitmap_idx = (page_addr / SIZE_OF_PAGE) % 64;
  bitmap_qword = (page_addr / SIZE_OF_PAGE) / 64;

  mask = 0x8000000000000000;
  mask >>= bitmap_idx;

  ASSERT(mask != 0);
  ASSERT(bitmap_qword < BITMAP_SIZE);

  phys_pages_bitmap[bitmap_qword] = phys_pages_bitmap[bitmap_qword] | mask;

  ASSERT(phys_pages_bitmap[bitmap_qword] != 0);

  KL_TRC_EXIT;
}

/// @brief Mark the page as in use in the bitmap.
///
/// Note that no checking is done to ensure the page is within the physical pages available to the system.
///
/// @param page_addr The address of the physical page to mark as in use.
void mem_clear_bitmap_page_bit(unsigned long page_addr)
{
  KL_TRC_ENTRY;

  unsigned long bitmap_qword;
  unsigned long bitmap_idx;
  unsigned long mask;

  ASSERT((page_addr / SIZE_OF_PAGE) < MAX_SUPPORTED_PAGES);

  bitmap_idx = (page_addr / SIZE_OF_PAGE) % 64;
  bitmap_qword = (page_addr / SIZE_OF_PAGE) / 64;

  mask = 0x8000000000000000;
  mask >>= bitmap_idx;

  ASSERT(mask != 0);
  ASSERT(bitmap_qword < BITMAP_SIZE);

  phys_pages_bitmap[bitmap_qword] = phys_pages_bitmap[bitmap_qword] & (~mask);

  KL_TRC_EXIT;
}

/// @brief Determine whether a specific page has its bit set in the pages bitmap.
///
/// Note that a true return value indicates the page is FREE.
///
/// @param page_addr The page address to check
///
/// @return TRUE implies the page is free (the bit is set), FALSE implies in use.
bool mem_is_bitmap_page_bit_set(unsigned long page_addr)
{
  KL_TRC_ENTRY;

  unsigned long bitmap_qword;
  unsigned long bitmap_idx;
  unsigned long mask;

  ASSERT((page_addr / SIZE_OF_PAGE) < MAX_SUPPORTED_PAGES);

  bitmap_idx = (page_addr / SIZE_OF_PAGE) % 64;
  bitmap_qword = (page_addr / SIZE_OF_PAGE) / 64;

  mask = 0x8000000000000000;
  mask >>= bitmap_idx;

  ASSERT(mask != 0);
  ASSERT(bitmap_qword < BITMAP_SIZE);

  KL_TRC_EXIT;

  return (phys_pages_bitmap[bitmap_qword] & mask);

}
