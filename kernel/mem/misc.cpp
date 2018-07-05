//#define ENABLE_TRACING

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/mem-int.h"

// Allocate the specified number of pages and map virtual addresses ready for
// use within the kernel.
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

// Unmap and deallocate a range of pages previously used within the kernel.
void mem_deallocate_pages(void *virtual_start, uint32_t num_pages)
{
  KL_TRC_ENTRY;

  ASSERT(num_pages != 0);
  ASSERT(((uint64_t)virtual_start) % MEM_PAGE_SIZE == 0);

  panic("mem_deallocate_pages Not implemented");

  KL_TRC_EXIT;
}

// Map a range of virtual addresses to an equally long range of physical
// addresses.
void mem_map_range(void *physical_start, void* virtual_start, unsigned int len, task_process *context, MEM_CACHE_MODES cache_mode)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Physical start address", physical_start, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Virtual start address", virtual_start, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Length", len, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Context", context, "\n");

  uint8_t *cur_virt_addr = (uint8_t *)virtual_start;
  uint8_t *cur_phys_addr = (uint8_t *)physical_start;
  int iterations = (len / MEM_PAGE_SIZE) + (len % MEM_PAGE_SIZE == 0 ? 0 : 1);

  ASSERT(((uint64_t)physical_start) % MEM_PAGE_SIZE == 0);
  ASSERT(((uint64_t)virtual_start) % MEM_PAGE_SIZE == 0);
  ASSERT(len > 0);

  for(int i = 0; i < iterations; i++)
  {
    mem_map_virtual_page((uint64_t)cur_virt_addr,
                         (uint64_t)cur_phys_addr,
                         context,
                         cache_mode);
    cur_virt_addr += MEM_PAGE_SIZE;
    cur_phys_addr += MEM_PAGE_SIZE;
  }

  KL_TRC_EXIT;
}

// Remove the link between a specified number of physical and virtual pages.
void mem_unmap_range(void *virtual_start, uint32_t num_pages)
{
  KL_TRC_ENTRY;

  uint8_t *cur_virt_addr = (uint8_t *)virtual_start;

  ASSERT (((uint64_t)virtual_start) % MEM_PAGE_SIZE == 0);

  for (int i = 0; i < num_pages; i++)
  {
    mem_unmap_virtual_page((uint64_t)cur_virt_addr);
    cur_virt_addr += MEM_PAGE_SIZE;
  }

  KL_TRC_EXIT;
}

mem_process_info *mem_task_get_task0_entry()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Returning task 0 data address: ", &task0_entry, "\n");

  KL_TRC_EXIT;

  return &task0_entry;
}
