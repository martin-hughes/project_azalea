/// @file
/// @brief Functions controlling non-architecture specific parts of mapping virtual addresses to physical pages.

//#define ENABLE_TRACING

#include "mem.h"
#include "mem-int.h"

// Known deficiencies
// - mem_map_virtual_page and mem_map_range have opposite parameter ordering!
// - Not all functions support process contexts.
// - mem_deallocate_pages is not complete.

namespace
{
  /// For each page in RAM, how many virtual pages refer to it? When this counter reaches zero, the physical page can
  /// probably be freed.
  uint32_t page_use_counters[MEM_MAX_SUPPORTED_PAGES];

  /// A lock to protect the whole counter and release system.
  ipc::raw_spinlock counter_lock;
}

/// Set the page use counter table to zero, since the only pages currently in use will never be unmapped.
///
void mem_map_init_counters()
{
  KL_TRC_ENTRY;

  ipc_raw_spinlock_init(counter_lock);
  ipc_raw_spinlock_lock(counter_lock);

  for (int i = 0; i < MEM_MAX_SUPPORTED_PAGES; i++)
  {
    page_use_counters[i] = 0;
  }

  ipc_raw_spinlock_unlock(counter_lock);

  KL_TRC_EXIT;
}

/// @brief Map a single virtual page to a single physical page.
///
/// @param virt_addr The address of the beginning of a page of virtual memory.
///
/// @param phys_addr The address of the beginning of a physical page that will back the virtual page.
///
/// @param context Which process is this mapping occurring in. If nullptr, assume the current process.
///
/// @param cache_mode The cache mode that should apply to this mapping. See MEM_CACHE_MODES for more.
void mem_map_virtual_page(uint64_t virt_addr,
                          uint64_t phys_addr,
                          task_process *context,
                          MEM_CACHE_MODES cache_mode)
{
  uint32_t phys_page_num;

  KL_TRC_ENTRY;

  mem_arch_map_virtual_page(virt_addr, phys_addr, context, cache_mode);

  ipc_raw_spinlock_lock(counter_lock);

  ASSERT((phys_addr % MEM_PAGE_SIZE) == 0);
  phys_page_num = phys_addr / MEM_PAGE_SIZE;
  if ((phys_page_num < MEM_MAX_SUPPORTED_PAGES) && (page_use_counters[phys_page_num] < 0xFFFFFFFF))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Increment counter for: ", phys_page_num);
    page_use_counters[phys_page_num]++;
    KL_TRC_TRACE(TRC_LVL::FLOW, " to: ", page_use_counters[phys_page_num], "\n");
  }

  ipc_raw_spinlock_unlock(counter_lock);

  KL_TRC_EXIT;
}

/// @brief Unmap a single virtual page so that the physical page that was backing it is no longer used.
///
/// @param virt_addr The virtual address to unmap.
///
/// @param context The process to do the unmapping in.
///
/// @param allow_phys_page_free If set to true, release any physical pages that are no longer referred to by a virtual
///                             page mapping. If false, do not release free pages. False is useful to stop the release
///                             and attempted re-use of, say, VGA video buffers.
void mem_unmap_virtual_page(uint64_t virt_addr, task_process *context, bool allow_phys_page_free)
{
  uint32_t phys_page_num;
  uint64_t phys_addr;

  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Considering virt_addr ", virt_addr, "\n");
  phys_addr = reinterpret_cast<uint64_t>(mem_get_phys_addr(reinterpret_cast<void *>(virt_addr), context));
  mem_arch_unmap_virtual_page(virt_addr, context);

  if (phys_addr != 0)
  {
    ipc_raw_spinlock_lock(counter_lock);
    ASSERT((phys_addr % MEM_PAGE_SIZE) == 0);
    phys_page_num = phys_addr / MEM_PAGE_SIZE;


    if (page_use_counters[phys_page_num] > 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Decrement counter for: ", phys_page_num);
      page_use_counters[phys_page_num]--;
      KL_TRC_TRACE(TRC_LVL::FLOW, " to: ", page_use_counters[phys_page_num], "\n");
    }

    if ((page_use_counters[phys_page_num] == 0) && allow_phys_page_free)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Deallocate page: ", virt_addr, "\n");
      mem_deallocate_physical_pages(reinterpret_cast<void *>(phys_addr), 1);
    }

    ipc_raw_spinlock_unlock(counter_lock);
  }

  KL_TRC_EXIT;
}

/// @brief Map a range of virtual addresses to an equally long range of physical addresses.
///
/// @param physical_start The address of the first physical page in the mapping. The physical pages must be contiguous.
///
/// @param virtual_start The address of the first virtual page in the mapping. The virtual pages must be contiguous.
///
/// @param len The number of consecutive pages to map.
///
/// @param context Which process is this mapping occurring in. If nullptr, assume the current process.
///
/// @param cache_mode The cache mode that should apply to this mapping. See MEM_CACHE_MODES for more.
void mem_map_range(void *physical_start,
                   void* virtual_start,
                   uint32_t len,
                   task_process *context,
                   MEM_CACHE_MODES cache_mode)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Physical start address", physical_start, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Virtual start address", virtual_start, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Length", len, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Context", context, "\n");

  uint8_t *cur_virt_addr = (uint8_t *)virtual_start;
  uint8_t *cur_phys_addr = (uint8_t *)physical_start;

  ASSERT(((uint64_t)physical_start) % MEM_PAGE_SIZE == 0);
  ASSERT(((uint64_t)virtual_start) % MEM_PAGE_SIZE == 0);
  ASSERT(len > 0);

  for(int i = 0; i < len; i++)
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

/// @brief Remove the link between a specified number of physical and virtual pages.
///
/// @param virtual_start The start of the first page in the range to unmap.
///
/// @param num_pages The length of the range to unmap.
///
/// @param context The process to do the unmapping in.
///
/// @param allow_phys_page_free If set to true, release any physical pages that are no longer referred to by a virtual
///                             page mapping. If false, do not release free pages. False is useful to stop the release
///                             and attempted re-use of, say, VGA video buffers.
void mem_unmap_range(void *virtual_start, uint32_t num_pages, task_process *context, bool allow_phys_page_free)
{
  KL_TRC_ENTRY;

  uint8_t *cur_virt_addr = (uint8_t *)virtual_start;

  ASSERT (((uint64_t)virtual_start) % MEM_PAGE_SIZE == 0);

  for (int i = 0; i < num_pages; i++)
  {
    mem_unmap_virtual_page(reinterpret_cast<uint64_t>(cur_virt_addr), context, allow_phys_page_free);
    cur_virt_addr += MEM_PAGE_SIZE;
  }

  KL_TRC_EXIT;
}
