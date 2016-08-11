#ifndef MEM_H_
#define MEM_H_

// Main kernel interface to memory management functions.  The mem module
// provides basic memory management at the level of pages, generally the klib
// memory functions should be used to allocate or deallocate specific amounts of
// memory.

void mem_gen_init();

// A structure to contain information specific to a single process.
// In future, this will be able to track things like allocation counts and so
// on, but for now it just contains the x64 specific data.
struct mem_process_info
{
    // Pointer to architecture-specific information about a specific process.
    // Opaque to any non-architecture specific code.
    void *arch_specific_data;
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

struct task_process;

// In theory, no other part of the kernel should define NULL, however because we
// include this file in some of the unit tests - which also include some of the
// standard library headers - it's possible that it'll be defined there before
// here, so guard against that.
#ifndef NULL
#define NULL ((void *)0)
#endif

// TODO: Consider how these interact with multiple possible process spaces. (MT)
void *mem_allocate_physical_pages(unsigned int num_pages);
void *mem_allocate_virtual_range(unsigned int num_pages);
void mem_map_range(void *physical_start,
                   void* virtual_start,
                   unsigned int len,
                   task_process *context = (task_process *)NULL,
                   MEM_CACHE_MODES cache_mode = MEM_WRITE_BACK);
void *mem_allocate_pages(unsigned int num_pages);

void mem_deallocate_physical_pages(void *start, unsigned int num_pages);
void mem_deallocate_virtual_range(void *start, unsigned int num_pages);
void mem_unmap_range(void *virtual_start, unsigned int num_pages);
void mem_deallocate_pages(void *virtual_start, unsigned int num_pages);

// A helper function to allow the task manager to easily find the information
// about task-0 memory.
mem_process_info *mem_task_get_task0_entry();

// Allow the task manager to create or destroy memory manager information as needed.
// Destroying a task entry will also cause any PTEs and mappings to be destroyed.
// These functions are part of the architecture-specific code, they fill in the generic information as needed.
mem_process_info *mem_task_create_task_entry();
void mem_task_destroy_task_entry();

// Useful definitions.
const unsigned long MEM_PAGE_SIZE = 2097152;

#endif /* MEM_H_ */
