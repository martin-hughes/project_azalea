/// @file
/// @brief Memory related functions that are not specific to virtual or physical memory managers.

// Known deficiencies:
// - Is mem_task_get_task0_entry() still necessary?

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/mem-int.h"
#include "mem/x64/mem-x64-int.h"

/// @brief Allocate the specified number of pages and map virtual addresses for use within the kernel.
///
/// The allocated pages form a contiguous block of virtual memory within kernel space. Each page is backed by a unique
/// physical page.
///
/// @param num_pages How many pages are required.
///
/// @return Pointer to the start of the allocated memory range.
void *mem_allocate_pages(uint32_t num_pages)
{
  KL_TRC_ENTRY;

  ASSERT(num_pages != 0);

  uint8_t *cur_virtual_addr;
  void *return_addr;
  void *cur_phys_addr;

  return_addr = mem_allocate_virtual_range(num_pages);
  cur_virtual_addr = (uint8_t *)return_addr;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Returned virtual address", return_addr, "\n");

  for (int i = 0; i < num_pages; i++, cur_virtual_addr += MEM_PAGE_SIZE)
  {
    cur_phys_addr = mem_allocate_physical_pages(1);
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Current phys addr", cur_phys_addr, "\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Current virt addr", cur_virtual_addr, "\n");
    mem_map_range(cur_phys_addr, cur_virtual_addr, MEM_PAGE_SIZE);
  }

  KL_TRC_EXIT;

  return return_addr;
}

/// @brief Unmap and deallocate a range of pages previously allocated by mem_allocate_pages()
///
/// @param virtual_start The start of the virtual range allocated by mem_allocate_pages()
///
/// @param The number of pages in the allocation being freed.
void mem_deallocate_pages(void *virtual_start, uint32_t num_pages)
{
  KL_TRC_ENTRY;

  ASSERT(num_pages != 0);
  ASSERT(((uint64_t)virtual_start) % MEM_PAGE_SIZE == 0);

  mem_unmap_range(virtual_start, num_pages, nullptr, true);

  KL_TRC_EXIT;
}
