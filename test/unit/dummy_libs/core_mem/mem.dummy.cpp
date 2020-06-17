// Dummy version of the core memory library. This can be used by test code that
// interacts with the kernel memory system, and it should behave plausibly for
// most test cases.
//
// It will have difficulty with code that allocates physical and virtual ranges
// and maps them to each other.

#include "test_core/test.h"
#include "processor.h"
#include "mem.h"
#include <malloc.h>
#include <iostream>
using namespace std;

const uint64_t page_size = 2 * 1024 * 1024;

uint32_t fake_arch_specific_info;
mem_process_info task0_entry = { &fake_arch_specific_info };

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

void *mem_get_phys_addr(void *virtual_addr, task_process *context)
{
  return nullptr;
}

bool mem_is_valid_virt_addr(uint64_t virtual_addr)
{
  // It's reasonable to assume 'yes' in the test code, because all allocations ultimately come from the OS.
  return true;
}

void mem_arch_map_virtual_page(uint64_t virt_addr,
                              uint64_t phys_addr,
                              task_process *context,
                              MEM_CACHE_MODES cache_mode)
{
  // In the test scripts this doesn't do anything, but scripts that rely on mapping will fail.
}

void mem_arch_unmap_virtual_page(uint64_t virt_addr, task_process *context)
{
  // as above.
}

struct process_x64_data;

void mem_x64_pml4_allocate(process_x64_data &new_proc_data)
{
  // This is always transparent to processes, so it can be ignored in the tests.
}

void mem_x64_pml4_deallocate(process_x64_data &new_proc_data)
{
  // as above.
}

void mem_arch_init_task_entry(mem_process_info *entry)
{
  // Nothing to do - there's no specific info for the unit tests.
}

void mem_arch_release_task_entry(mem_process_info *entry)
{
  // ... and thus nothing to release.
}
