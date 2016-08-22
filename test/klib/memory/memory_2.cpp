// Klib-memory test script 1.
//
// Simple allocation tests, testing various sizes of allocation and free.
// More complex tests to prove that the allocator works as expected are covered
// by later tests.

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <time.h>

#include "klib/memory/memory.h"
#include "test/test_core/test.h"
#include "memory_test_list.h"

using namespace std;

struct allocation
{
  void *ptr;
  unsigned long size;
};

typedef std::vector<allocation> allocation_list;

const unsigned long MAX_ALLOCATIONS = 1000;
const unsigned long MAX_SINGLE_CHUNK = 262144;
const unsigned long ITERATIONS = 1000000;

allocation_list completed_allocations;
unsigned long allocation_count;

void memory_test_2()
{
  test_only_reset_allocator();

  srand (time(nullptr));

  bool allocate = false;
  float proportion;
  unsigned long bytes_to_allocate;
  unsigned long dealloc_idx;
  allocation this_allocation;

  for (int i = 0; i < ITERATIONS; i++)
  {
    // Decide whether to do an allocation or deallocation. When deciding, note
    // that it's impossible to deallocate if there's nothing to deallocate, and
    // we don't want to just keep filling the list forever.
    allocate = rand() % 2;
    if (allocation_count >= MAX_ALLOCATIONS)
    {
      allocate = false;
    }
    if (allocation_count == 0)
    {
      allocate = true;
    }

    if (allocate)
    {
      // Decide how much to allocate.
      proportion = (float)rand() / RAND_MAX;
      bytes_to_allocate = (unsigned long)(proportion * MAX_SINGLE_CHUNK);

      // Then allocate it. Store it in the list so it can be deallocated later
      // on.
      this_allocation.size = bytes_to_allocate;
      this_allocation.ptr = kmalloc(bytes_to_allocate);
      completed_allocations.push_back(this_allocation);
      allocation_count++;
      //* cout << "Allocated " << std::dec << this_allocation.size
      //*     << " bytes @ 0x" << std::hex << this_allocation.ptr << endl;
    }
    else
    {
      // Get a random allocation and deallocate it.
      dealloc_idx = rand() % allocation_count;
      allocation_count--;

      this_allocation = completed_allocations.at(dealloc_idx);
      completed_allocations.erase(completed_allocations.begin() + dealloc_idx);

      //* cout << "Deallocating " << std::dec << this_allocation.size <<
      //*      " bytes @ 0x" << std::hex << this_allocation.ptr << endl;

      kfree(this_allocation.ptr);
    }
  }
}
