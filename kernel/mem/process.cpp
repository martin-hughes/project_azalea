/// @file
/// @brief Memory related functions that are not specific to virtual or physical memory managers.

//#define ENABLE_TRACING

#include "mem.h"
#include "mem-int.h"
#include "processor.h"

/// @brief Returns a mem_info block that is created at compile time, to avoid allocating one during system startup.
///
/// This is helpful because the allocation can throw the system into a circle of allocations.
///
/// @return The address of the statically allocated process memory info for the kernel process.
mem_process_info *mem_task_get_task0_entry()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Returning task 0 data address: ", &task0_entry, "\n");

  KL_TRC_EXIT;

  return &task0_entry;
}

/// @brief Create the memory manager specific part of a process's information block.
///
/// Fill in a process memory information struct, which is provided back to the task manager, for it to live with all
/// the other process information.
///
/// @return A new, filled in, mem_process_info block. It is the callers responsibility to call mem_task_free_task when
///         it is no longer needed.
mem_process_info *mem_task_create_task_entry()
{
  KL_TRC_ENTRY;

  mem_process_info *new_proc_info;

  new_proc_info = new mem_process_info;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Created new memory manager information at", new_proc_info, "\n");

  mem_arch_init_task_entry(new_proc_info);
  mem_vmm_init_proc_data(new_proc_info->process_vmm_data);

  KL_TRC_EXIT;
  return new_proc_info;
}

/// @brief Destroy a task's memory manager information block and releases all physical pages unique to this process.
///
/// @param proc The process to free information for.
void mem_task_free_task(task_process *proc)
{
  KL_TRC_ENTRY;

  ASSERT(proc != nullptr);
  ASSERT(proc->mem_info != nullptr);

  if (proc->mem_info != mem_task_get_task0_entry())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Delete task info\n");

    mem_vmm_free_proc_data(proc);
    mem_arch_release_task_entry(proc->mem_info);

    delete proc->mem_info;
    proc->mem_info = nullptr;
  }

  KL_TRC_EXIT;
}
