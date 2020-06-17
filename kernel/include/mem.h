/// @file
/// @brief Generic memory handling interface.

#pragma once

#include <stdint.h>

// Main kernel interface to memory management functions.  The mem module
// provides basic memory management at the level of pages, generally the klib
// memory functions should be used to allocate or deallocate specific amounts of
// memory.

// To include the constant MEM_PAGE_SIZE
#include "azalea/system_properties.h"

// To define spinlocks
#include "types/spinlock.h"

// To define lists
#include "types/list.h"

class task_process;
class task_thread;

/// @brief Stores information about whether a specific address range is allocated or not.
struct vmm_range_data
{
  uint64_t start; ///< The start address of the range being considered.
  uint64_t number_of_pages; ///< The number of pages in the range (must be a power of two).
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
  ipc::raw_spinlock vmm_lock;

  /// @brief The thread that is currently accessing this process's VMM data.
  task_thread *vmm_user_thread_id;
};

/// @brief A structure to contain information specific to a single process.
///
struct mem_process_info
{
  /// @brief Pointer to architecture-specific information about a specific process.
  ///
  /// Opaque to any non-architecture specific code.
  void *arch_specific_data;

  /// @brief Virtual Memory Manager data corresponding to this process.
  ///
  vmm_process_data process_vmm_data;
};

/// Selectable caching modes for users of the memory system. Yes, these are very similar to the constants in
/// MEM_X64_CACHE_TYPES - it saves having an extra translation while only the x64 architecture is supported.
///
/// The values are given in the Intel documentation, so are not covered by the Doxygen docs.
enum MEM_CACHE_MODES
{
  /// @cond
  MEM_UNCACHEABLE = 0,
  MEM_WRITE_COMBINING = 1,
  MEM_WRITE_THROUGH = 4,
  MEM_WRITE_PROTECTED = 5,
  MEM_WRITE_BACK = 6,
  /// @endcond
};

#pragma pack(push,1)
/// @brief A single record within an E820 memory map.
///
struct e820_record
{
  uint32_t size; ///< The size of this record.
  uint64_t start_addr; ///< The physical start address of this record.
  uint64_t length; ///< The length of this record.
  uint32_t memory_type; ///< The type of memory associated with this record.
};
#pragma pack(pop)

static_assert(sizeof(e820_record) == 24, "e820 record size wrong");

/// @brief Pointer to an 'E820' memory map.
///
/// This is usually provided by a multiboot compliant bootloader.
struct e820_pointer
{
  e820_record *table_ptr; ///< Pointer to the start of the e820 table - a physical address.
  unsigned int table_length; ///< The number of bytes in the table.
};

///////////////////////////////////////////////////////////////////////////////
// Platform independent functionality.                                       //
///////////////////////////////////////////////////////////////////////////////

void *mem_allocate_physical_pages(uint32_t num_pages);
void *mem_allocate_virtual_range(uint32_t num_pages, task_process *process_to_use = nullptr);
uint64_t mem_get_virtual_allocation_size(uint64_t start_addr, task_process *context);
void mem_vmm_allocate_specific_range(uint64_t start_addr, uint32_t num_pages, task_process *process_to_use);
void mem_map_range(void *physical_start,
                   void* virtual_start,
                   uint32_t len,
                   task_process *context = nullptr,
                   MEM_CACHE_MODES cache_mode = MEM_WRITE_BACK);
void *mem_allocate_pages(uint32_t num_pages);

void mem_deallocate_physical_pages(void *start, uint32_t num_pages);
void mem_deallocate_virtual_range(void *start, uint32_t num_pages, task_process *process_to_use = nullptr);
void mem_unmap_range(void *virtual_start, uint32_t num_pages, task_process *context, bool allow_phys_page_free);
void mem_deallocate_pages(void *virtual_start, uint32_t num_pages);

// A helper function to allow the task manager to easily find the information
// about task-0 memory.
mem_process_info *mem_task_get_task0_entry();

// Allow the task manager to create or destroy memory manager information as needed.
// Destroying a task entry will also cause any PTEs and mappings to be destroyed.
// These functions are part of the architecture-specific code, they fill in the generic information as needed.
mem_process_info *mem_task_create_task_entry();
void mem_task_free_task(task_process *proc);

// If we're building the unit test program on Windows then add the Microsoft Debug Heap instrumentation to any source
// file using KLib. This won't instrument *everything* - but it's close enough.
#ifdef UT_MEM_LEAK_CHECK

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#define new DEBUG_NEW

#endif

void *kmalloc(uint64_t mem_size);
void kfree(void *mem_block);

uint64_t kl_mem_block_size(void *ptr);

// Only for use by test code. See the associated comment in memory.cpp for
// details.
#ifdef AZALEA_TEST_CODE
void test_only_reset_allocator();
#endif

// Useful memory-related helper functions.
void klib_mem_split_addr(uint64_t base_addr, uint64_t &page_addr, uint64_t &page_offset);

///////////////////////////////////////////////////////////////////////////////
// Architecture specific part                                                //
///////////////////////////////////////////////////////////////////////////////
void mem_gen_init(e820_pointer *e820_ptr);
void mem_free_startup_mem();
void *mem_get_phys_addr(void *virtual_addr, task_process *context = nullptr);
bool mem_is_valid_virt_addr(uint64_t virtual_addr);

/// @brief Invalidate the page table TLB on the calling processor.
extern "C" void mem_invalidate_tlb();
