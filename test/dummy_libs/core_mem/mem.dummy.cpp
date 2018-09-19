// Dummy version of the core memory library. This can be used by test code that
// interacts with the kernel memory system, and it should behave plausibly for
// most test cases.
//
// It will have difficulty with code that allocates physical and virtual ranges
// and maps them to each other.

#include "test/test_core/test.h"
#include "processor/processor.h"
#include "mem/mem.h"
#include <malloc.h>
#include <iostream>
using namespace std;

const uint64_t page_size = 2 * 1024 * 1024;

// In the dummy library, this doesn't need to do anything. All set up is done
// automatically when this test code gets this far, which means the tests don't
// need to worry about starting up this libary.
void mem_gen_init()
{
  panic("mem_gen_init not written");
}

void *mem_allocate_physical_pages(uint32_t num_pages)
{
  panic("mem_allocate_physical_pages not implemented");
  return nullptr;
}

void *mem_allocate_virtual_range(uint32_t num_pages, task_process *process_to_use)
{
  panic("mem_allocate_virtual_range Not implemented");
  return nullptr;
}

void mem_map_range(void *physical_start,
                   void* virtual_start,
                   uint32_t len,
                   task_process *context,
                   MEM_CACHE_MODES cache_mode)
{
  panic("mem_map_range Not implemented");
}

// Allocate pages of RAM. Some of the kernel code relies on the assumption that
// the returned address is aligned on page boundaries so use memalign for that.
void *mem_allocate_pages(uint32_t num_pages)
{
  uint64_t requested_ram = num_pages * page_size;

#ifdef _MSVC_LANG
  return _aligned_malloc(requested_ram, page_size);
#else
  return memalign(page_size, requested_ram);
#endif
}

void mem_deallocate_physical_pages(void *start, uint32_t num_pages)
{
  panic("mem_deallocate_physical_pages Not implemented");
}

void mem_deallocate_virtual_range(void *start, uint32_t num_pages)
{
  panic("mem_deallocate_virtual_range Not implemented");
}

void mem_unmap_range(void *virtual_start, uint32_t num_pages)
{
  panic("mem_unmap_range Not implemented");
}

void mem_deallocate_pages(void *virtual_start, uint32_t num_pages)
{
#ifndef _MSVC_LANG
  free(virtual_start);
#else
  _aligned_free(virtual_start);
#endif
}

void mem_vmm_allocate_specific_range(uint64_t start_addr, uint32_t num_pages, task_process *process_to_use)
{
  // Do nothing.
}

void *mem_get_phys_addr(void *virtual_addr, task_process *context)
{
  panic("mem_get_phys_addr not implemented");
  return nullptr;
}

bool mem_is_valid_virt_addr(uint64_t virtual_addr)
{
  // It's reasonable to assume 'yes' in the test code, because all allocations ultimately come from the OS.
  return true;
}