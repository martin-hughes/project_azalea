/// @file
/// @brief Memory management part of the system call interface

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "mem/mem.h"
#include "object_mgr/object_mgr.h"

// Known defects:
// - It isn't possible to deallocate virtual memory, since the kernel doesn't really track the allocations properly
//   yet.
// - Having run out of RAM in syscall_allocate_backing_memory, we should deallocate some rather than just sitting tight.
// - Attempting to double-map a virtual range causes a kernel panic.
// - The way that VMM requires power-of-two sizes might cause trouble one day.

/// @brief Back a virtual address range in the calling process with physical RAM.
///
/// This function will allocate physical RAM to back this allocation. Processes are responsible for managing their own
/// virtual address spaces, so map_addr is an input, not an output.
///
/// @param pages The number of pages to allocate.
///
/// @param map_addr Pointer to the beginning of the range to be backed with RAM.
///
/// @return ERR_CODE::NO_ERROR if the allocated succeeded. ERR_CODE::INVALID_PARAM if the length is zero, or
///         `map_addr` does not point to a valid memory range. ERR_CODE::INVALID_OP if this virtual address range is
///         already mapped. ERR_CODE::OUT_OF_RESOURCE if the system has run out of physical memory.
ERR_CODE syscall_allocate_backing_memory(unsigned long pages, void **map_addr)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  unsigned long map_addr_start = reinterpret_cast<unsigned long>(*map_addr);
  unsigned long map_addr_end = map_addr_start + (pages * MEM_PAGE_SIZE);
  unsigned long cur_map_addr;
  void *phys_page;
  task_thread *cur_thread;

  KL_TRC_ENTRY;

  if ((pages == 0) ||
      (pages > 0x8000000000000000) ||
      map_addr == nullptr ||
      !SYSCALL_IS_UM_ADDRESS(map_addr) ||
      !SYSCALL_IS_UM_ADDRESS(map_addr_end) ||
      !SYSCALL_IS_UM_ADDRESS(*map_addr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid params\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    result = ERR_CODE::NO_ERROR;

    if (*map_addr == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "App requests random assignment of ", pages, " pages\n");
      cur_thread = task_get_cur_thread();
      ASSERT(cur_thread != nullptr);
      ASSERT(cur_thread->parent_process != nullptr);
      *map_addr = mem_allocate_virtual_range(pages, cur_thread->parent_process);

      KL_TRC_TRACE(TRC_LVL::FLOW, "Proposed space: ", *map_addr, "\n");
    }

    map_addr_start = reinterpret_cast<unsigned long>(*map_addr);
    map_addr_end = map_addr_start + (pages * MEM_PAGE_SIZE);
    cur_map_addr = map_addr_start;

    for (int i = 0; i < pages; i++, cur_map_addr += MEM_PAGE_SIZE)
    {
      if (mem_get_phys_addr(reinterpret_cast<void *>(cur_map_addr)) != nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Attempted duplicate mapping @ ", cur_map_addr, "\n");
        result = ERR_CODE::INVALID_OP;
        break;
      }
    }

    cur_map_addr = map_addr_start;

    if (result == ERR_CODE::NO_ERROR)
    {
      for (int i = 0; i < pages; i++, cur_map_addr += MEM_PAGE_SIZE)
      {
        phys_page = mem_allocate_physical_pages(1);
        if (phys_page == nullptr)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Ran out of pages\n");
          result = ERR_CODE::OUT_OF_RESOURCE;
        }
        else
        {
          mem_map_range(phys_page, reinterpret_cast<void *>(cur_map_addr), 1);
        }
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Deallocate a virtual memory range from the requesting process.
///
/// At present this function is not working, since the kernel doesn't really track virtual allocations by process...
///
/// @param dealloc_ptr Pointer to the beginning of the range to deallocate.
///
/// @return ERR_CODE::INVALID_OP in all cases, until the kernel works better.
ERR_CODE syscall_release_backing_memory(void *dealloc_ptr)
{
  return ERR_CODE::INVALID_OP;
}

/// @brief Map a memory range to be shared between two processes.
///
/// Maps a memory range so that it is shared between two processes. This ultimately means that both processes are
/// writing and reading the same chunk of physical memory, even though they do not necessarily use the same virtual
/// addresses to refer to that RAM.
///
/// The mapping will fail if the range is already allocated in the process the mapping is being executed in.
///
/// @param proc_mapping_in Handle to the process that doesn't currently have the mapping in it, i.e. we are executing
///                        the mapping operation in the context of this process. A value of zero indicates this
///                        process.
///
/// @param map_addr The address that the shared memory should have in `proc_mapping_in`. This must start on a page
///                 boundary.
///
/// @param length The number of bytes of shared memory to create. This must be a multiple of MEM_PAGE_SIZE.
///
/// @param proc_already_in Handle to the process that already has the memory mapped in it. A value of zero indicates
///                        this process.
///
/// @param extant_addr Pointer to the soon-to-be-shared memory in `proc_already_in`. This must start on a page
///                    boundary.
///
/// @return ERR_CODE::INVALID_PARAM if either process handle or memory address is invalid or if the mapping length is
///         not a multiple of the page size. ERR_CODE::INVALID_OP if the memory is already mapped in the receiving
///         process. ERR_CODE::NO_ERROR if the mapping succeeded.
ERR_CODE syscall_map_memory(GEN_HANDLE proc_mapping_in,
                            void *map_addr,
                            unsigned long length,
                            GEN_HANDLE proc_already_in,
                            void *extant_addr)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  task_process *receiving_proc;
  task_process *originating_proc;
  unsigned long map_addr_l = reinterpret_cast<unsigned long>(map_addr);
  unsigned long extant_addr_l = reinterpret_cast<unsigned long>(extant_addr);
  void *phys_addr;

  KL_TRC_ENTRY;

  if (((length % MEM_PAGE_SIZE) != 0) ||
      (map_addr == nullptr) ||
      (extant_addr == nullptr) ||
      !SYSCALL_IS_UM_ADDRESS(map_addr) ||
      !SYSCALL_IS_UM_ADDRESS(extant_addr) ||
      ((map_addr_l % MEM_PAGE_SIZE) != 0) ||
      ((extant_addr_l % MEM_PAGE_SIZE) != 0))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid params\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    if (proc_mapping_in != 0)
    {
      receiving_proc = dynamic_cast<task_process *>(om_retrieve_object(proc_mapping_in));
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Map in this process\n");
      receiving_proc = task_get_cur_thread()->parent_process;
    }

    if (proc_already_in != 0)
    {
      originating_proc = dynamic_cast<task_process *>(om_retrieve_object(proc_already_in));
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Map from this process\n");
      originating_proc = task_get_cur_thread()->parent_process;
    }

    if ((receiving_proc == nullptr) || (originating_proc == nullptr))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid handles\n");
      result = ERR_CODE::INVALID_PARAM;
    }
    else
    {
      for (int i = 0;
           i < (length / MEM_PAGE_SIZE);
           i++, map_addr_l += MEM_PAGE_SIZE)
      {
        phys_addr = mem_get_phys_addr(reinterpret_cast<void *>(map_addr_l), receiving_proc);
        if (phys_addr != nullptr)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Duplicate allocation attempt\n");
          result = ERR_CODE::INVALID_OP;
        }
      }

      if (result != ERR_CODE::INVALID_OP)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt allocation\n");
        map_addr_l = reinterpret_cast<unsigned long>(map_addr);

        for (int i = 0;
            i < (length / MEM_PAGE_SIZE);
            i++, extant_addr_l += MEM_PAGE_SIZE, map_addr_l += MEM_PAGE_SIZE)
        {
          phys_addr = mem_get_phys_addr(reinterpret_cast<void *>(extant_addr_l), originating_proc);
          mem_map_range(phys_addr, reinterpret_cast<void *>(map_addr_l), 1, receiving_proc);
        }

        result = ERR_CODE::NO_ERROR;
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Unmap a virtual memory range.
///
/// This capability isn't operative yet.
///
/// @return ERR_CODE::INVALID_OP in all cases, until the kernel works better.
ERR_CODE syscall_unmap_memory()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return ERR_CODE::INVALID_OP;
}
