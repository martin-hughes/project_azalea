// Dummy version of the core memory library. This can be used by test code that
// interacts with the kernel memory system, and it should behave plausibly for
// most test cases.
//
// It will have difficulty with code that allocates physical and virtual ranges
// and maps them to each other.

#include "mem/mem.h"
#include "test/test_core/test.h"

#include <malloc.h>
#include <iostream>
using namespace std;

const unsigned long page_size = 2 * 1024 * 1024;

// In the dummy library, this doesn't need to do anything. All set up is done
// automatically when this test code gets this far, which means the tests don't
// need to worry about starting up this libary.
void mem_gen_init()
{
  panic("mem_gen_init not written");
}

void *mem_allocate_physical_pages(unsigned int num_pages)
{
  panic("mem_allocate_physical_pages not implemented");
  return nullptr;
}

void *mem_allocate_virtual_range(unsigned int num_pages)
{
  panic("mem_allocate_virtual_range Not implemented");
  return nullptr;
}

void mem_map_range(void *physical_start, void* virtual_start, unsigned int len)
{
  panic("mem_map_range Not implemented");
}

// Allocate pages of RAM. Some of the kernel code relies on the assumption that
// the returned address is aligned on page boundaries so use memalign for that.
void *mem_allocate_pages(unsigned int num_pages)
{
  unsigned long requested_ram = num_pages * page_size;
  return memalign(page_size, requested_ram);
}

void mem_deallocate_physical_pages(void *start, unsigned int num_pages)
{
  panic("mem_deallocate_physical_pages Not implemented");
}

void mem_deallocate_virtual_range(void *start, unsigned int num_pages)
{
  panic("mem_deallocate_virtual_range Not implemented");
}

void mem_unmap_range(void *virtual_start, unsigned int num_pages)
{
  panic("mem_unmap_range Not implemented");
}

void mem_deallocate_pages(void *virtual_start, unsigned int num_pages)
{
  free(virtual_start);
}

void mem_vmm_allocate_specific_range(unsigned long start_addr, unsigned int num_pages, task_process *process_to_use)
{
  // Do nothing.
}