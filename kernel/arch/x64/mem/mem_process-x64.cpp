/// @file
/// @brief Functions to manage x64 specific memory manager data about processes.

#include "../mem/mem-int.h"
#include "mem-x64.h"

void mem_arch_init_task_entry(mem_process_info *entry)
{
  process_x64_data *new_x64_proc_info;

  KL_TRC_ENTRY;

  ASSERT(entry != nullptr);
  new_x64_proc_info = new process_x64_data;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Created new x64 information at", new_x64_proc_info, "\n");

  mem_x64_pml4_allocate(*new_x64_proc_info);
  entry->arch_specific_data = reinterpret_cast<void *>(new_x64_proc_info);

  KL_TRC_EXIT;
}

void mem_arch_release_task_entry(mem_process_info *entry)
{
  process_x64_data *x64_proc_info;
  KL_TRC_ENTRY;

  ASSERT(entry != nullptr);
  x64_proc_info = reinterpret_cast<process_x64_data *>(entry->arch_specific_data);
  ASSERT(x64_proc_info != nullptr);

  mem_x64_pml4_deallocate(*x64_proc_info);
  delete x64_proc_info;

  entry->arch_specific_data = nullptr;

  KL_TRC_EXIT;
}
