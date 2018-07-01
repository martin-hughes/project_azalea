#ifndef MEM_H_
#define MEM_H_

// Main kernel interface to memory management functions.  The mem module
// provides basic memory management at the level of pages, generally the klib
// memory functions should be used to allocate or deallocate specific amounts of
// memory.

// To include the constant MEM_PAGE_SIZE
#include "user_interfaces/system_properties.h"

// To define spinlocks
#include "klib/synch/kernel_locks.h"

// To define lists
#include "klib/data_structures/lists.h"

struct task_process;
struct task_thread;

/// @brief Stores information about whether a specific address range is allocated or not.
struct vmm_range_data
{
  unsigned long start; ///< The start address of the range being considered.
  unsigned long number_of_pages; ///< The number of pages in the range (must be a power of two).
  bool allocated; ///< Whether or not this address range is allocated (true) or not (false).
};

/// @brief Store information about the allocations within a single process.
///
/// This structure is also used to store information about the kernel's internal memory allocations - the kernel being
/// treated a bit like a separate process.
struct vmm_process_data
{
  /// @brief List containing range items covering the address space of the process.
  klib_list<vmm_range_data *> vmm_range_data_list;

  /// @brief Lock protecting this process's VMM information.
  ///
  /// This lock permits only one thread to access the VMM at a time. However, since this code is re-entrant, it is
  /// necessary to store the thread ID of the owning thread as well, so that the thread doesn't try to claim a lock it
  /// already owns.
  kernel_spinlock vmm_lock;

  /// @brief The thread that is currently accessing this process's VMM data.
  task_thread *vmm_user_thread_id;
};

// A structure to contain information specific to a single process.
// In future, this will be able to track things like allocation counts and so
// on, but for now it just contains the x64 specific data.
struct mem_process_info
{
  // Pointer to architecture-specific information about a specific process.
  // Opaque to any non-architecture specific code.
  void *arch_specific_data;

  vmm_process_data process_vmm_data;
};

// Selectable caching modes for users of the memory system. Yes, these are very similar to the constants in
// MEM_X64_CACHE_TYPES - it saves having an extra translation while only the x64 architecture is supported.
enum MEM_CACHE_MODES
{
  MEM_UNCACHEABLE = 0,
  MEM_WRITE_COMBINING = 1,
  MEM_WRITE_THROUGH = 4,
  MEM_WRITE_PROTECTED = 5,
  MEM_WRITE_BACK = 6,
};

#pragma pack(push,1)
struct e820_record
{
  unsigned int size;
  unsigned long start_addr;
  unsigned long length;
  unsigned int memory_type;
};
#pragma pack(pop)

static_assert(sizeof(e820_record) == 24, "e820 record size wrong");

struct e820_pointer
{
  e820_record *table_ptr;
  unsigned int table_length;
};

void mem_gen_init(e820_pointer *e820_ptr);

void *mem_allocate_physical_pages(unsigned int num_pages);
void *mem_allocate_virtual_range(unsigned int num_pages, task_process *process_to_use = nullptr);
void mem_vmm_allocate_specific_range(unsigned long start_addr, unsigned int num_pages, task_process *process_to_use);
void mem_map_range(void *physical_start,
                   void* virtual_start,
                   unsigned int len,
                   task_process *context = nullptr,
                   MEM_CACHE_MODES cache_mode = MEM_WRITE_BACK);
void *mem_allocate_pages(unsigned int num_pages);

void mem_deallocate_physical_pages(void *start, unsigned int num_pages);
void mem_deallocate_virtual_range(void *start, unsigned int num_pages, task_process *process_to_use = nullptr);
void mem_unmap_range(void *virtual_start, unsigned int num_pages);
void mem_deallocate_pages(void *virtual_start, unsigned int num_pages);
void *mem_get_phys_addr(void *virtual_addr, task_process *context = nullptr);

bool mem_is_valid_virt_addr(unsigned long virtual_addr);

// A helper function to allow the task manager to easily find the information
// about task-0 memory.
mem_process_info *mem_task_get_task0_entry();

// Allow the task manager to create or destroy memory manager information as needed.
// Destroying a task entry will also cause any PTEs and mappings to be destroyed.
// These functions are part of the architecture-specific code, they fill in the generic information as needed.
mem_process_info *mem_task_create_task_entry();
void mem_task_destroy_task_entry();


#endif /* MEM_H_ */
