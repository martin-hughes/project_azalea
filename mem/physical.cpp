// The kernel's physical memory management system. It is fairly simple - pages
// are marked as allocated or deallocated in a bitmap. Requests for pages are
// satisfied from that.
//
// Inevitably, this will lead to issues with fragmentation, but these can be
// dealt with later.
//
// TODO: Make the allocator smarter generally. The slab allocator is so much
// better than this one!

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/mem-int.h"

// In the page allocation bitmap, a 1 indicates that the page is FREE.
const unsigned long MAX_SUPPORTED_PAGES = 2048;
const unsigned long SIZE_OF_PAGE = 2097152;
const unsigned long BITMAP_SIZE = MAX_SUPPORTED_PAGES / 64;
unsigned long phys_pages_bitmap[BITMAP_SIZE];
unsigned long free_pages;

// Initialise the physical memory management subsystem.
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

  ASSERT(free_pages > 0);

  KL_TRC_EXIT;
}

// Allocate a number of physical pages to the caller.
void *mem_allocate_physical_pages(unsigned int num_pages)
{
  KL_TRC_ENTRY;

  unsigned long mask;
  unsigned long addr;

  // For the time being, only allow the allocation of single pages.
  ASSERT(num_pages == 1);

  // Spin through the list, looking for a free page. Upon finding one, mark it
  // as in use and return the relevant address.
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

        KL_TRC_TRACE((TRC_LVL_EXTRA, "Address found\n"));
        KL_TRC_EXIT;
        return (void *)addr;
      }
      mask = mask >> 1;
    }
  }

  KL_TRC_EXIT;

  panic("No free pages to allocate.");
  return NULL;
}

// Simply clear the relevant flag in the free pages bitmap. There isn't anything
// yet to stop someone deallocating a page that doesn't actually exist.
//
// TODO: Should probably fix this - it could be used to crash the system. (STAB)
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

// Set a bit in the pages bitmap. NOTE: This corresponds to marking the page
// FREE.
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

// Clear a bit in the pages bitmap. Note that this corresponds to marking a page
// IN USE.
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

// Determine whether a specific page has its bit set in the pages bitmap. Note
// that a true return value indicates the page is FREE.
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
