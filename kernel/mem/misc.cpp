//#define ENABLE_TRACING

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/mem-int.h"

// Allocate the specified number of pages and map virtual addresses ready for
// use within the kernel.
void *mem_allocate_pages(unsigned int num_pages)
{
  KL_TRC_ENTRY;

  ASSERT(num_pages != 0);

  unsigned char *cur_virtual_addr;
  void *return_addr;
  void *cur_phys_addr;

  return_addr = mem_allocate_virtual_range(num_pages);
  cur_virtual_addr = (unsigned char *)return_addr;

  KL_TRC_DATA("Returned virtual address", (unsigned long)return_addr);

  for (int i = 0; i < num_pages; i++, cur_virtual_addr += MEM_PAGE_SIZE)
  {
    cur_phys_addr = mem_allocate_physical_pages(1);
    KL_TRC_DATA("Current phys addr", (unsigned long)cur_phys_addr);
    KL_TRC_DATA("Current virt addr", (unsigned long)cur_virtual_addr);
    mem_map_range(cur_phys_addr, cur_virtual_addr, MEM_PAGE_SIZE);
  }

  KL_TRC_EXIT;

  return return_addr;
}

// Unmap and deallocate a range of pages previously used within the kernel.
void mem_deallocate_pages(void *virtual_start, unsigned int num_pages)
{
  KL_TRC_ENTRY;

  ASSERT(num_pages != 0);
  ASSERT(((unsigned long)virtual_start) % MEM_PAGE_SIZE == 0);

  panic("mem_deallocate_pages Not implemented");

  KL_TRC_EXIT;
}

// Map a range of virtual addresses to an equally long range of physical
// addresses.
void mem_map_range(void *physical_start, void* virtual_start, unsigned int len, task_process *context, MEM_CACHE_MODES cache_mode)
{
  KL_TRC_ENTRY;

  KL_TRC_DATA("Physical start address", (unsigned long)physical_start);
  KL_TRC_DATA("Virtual start address", (unsigned long)virtual_start);
  KL_TRC_DATA("Length", (unsigned long)len);
  KL_TRC_DATA("Context", (unsigned long)context);

  unsigned char *cur_virt_addr = (unsigned char *)virtual_start;
  unsigned char *cur_phys_addr = (unsigned char *)physical_start;
  int iterations = (len / MEM_PAGE_SIZE) + (len % MEM_PAGE_SIZE == 0 ? 0 : 1);

  ASSERT(((unsigned long)physical_start) % MEM_PAGE_SIZE == 0);
  ASSERT(((unsigned long)virtual_start) % MEM_PAGE_SIZE == 0);
  ASSERT(len > 0);

  for(int i = 0; i < iterations; i++)
  {
    mem_map_virtual_page((unsigned long)cur_virt_addr,
                         (unsigned long)cur_phys_addr,
                         context,
                         cache_mode);
    cur_virt_addr += MEM_PAGE_SIZE;
    cur_phys_addr += MEM_PAGE_SIZE;
  }

  KL_TRC_EXIT;
}

// Remove the link between a specified number of physical and virtual pages.
void mem_unmap_range(void *virtual_start, unsigned int num_pages)
{
  KL_TRC_ENTRY;

  unsigned char *cur_virt_addr = (unsigned char *)virtual_start;

  ASSERT (((unsigned long)virtual_start) % MEM_PAGE_SIZE == 0);

  for (int i = 0; i < num_pages; i++)
  {
    mem_unmap_virtual_page((unsigned long)cur_virt_addr);
    cur_virt_addr += MEM_PAGE_SIZE;
  }

  KL_TRC_EXIT;
}

mem_process_info *mem_task_get_task0_entry()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Returning task 0 data address: ", (unsigned long)&task0_entry, "\n");

  KL_TRC_EXIT;

  return &task0_entry;
}
