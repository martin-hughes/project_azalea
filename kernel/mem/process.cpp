/// @file
/// @brief Memory related functions that are not specific to virtual or physical memory managers.

// Known deficiencies:
// - the task info blocks refer to x64.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/mem-int.h"
#include "mem/x64/mem-x64-int.h"
#include "processor/processor.h"

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
  process_x64_data *new_x64_proc_info;

  new_proc_info = new mem_process_info;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Created new memory manager information at", new_proc_info, "\n");

  new_x64_proc_info = new process_x64_data;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Created new x64 information at", new_x64_proc_info, "\n");

  mem_x64_pml4_allocate(*new_x64_proc_info);
  mem_vmm_init_proc_data(new_proc_info->process_vmm_data);

  new_proc_info->arch_specific_data = (void *)new_x64_proc_info;

  KL_TRC_EXIT;
  return new_proc_info;
}

/// @brief Destroy a task's memory manager information block and releases all physical pages unique to this process.
///
/// @param proc The process to free information for.
void mem_task_free_task(task_process *proc)
{
  process_x64_data *x64_data;

  KL_TRC_ENTRY;

  ASSERT(proc != nullptr);
  x64_data = reinterpret_cast<process_x64_data *>(proc->mem_info->arch_specific_data);
  ASSERT(x64_data != nullptr);

  if (proc->mem_info != mem_task_get_task0_entry())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Delete task info\n");

    mem_vmm_free_proc_data(proc);
    mem_x64_pml4_deallocate(*x64_data);

    delete x64_data;
    delete proc->mem_info;
    proc->mem_info = nullptr;
  }

  KL_TRC_EXIT;
}
