// Klib-memory test script 2.
//
// Contains two tests that fuzz the Klib allocator by randomly allocating and deallocating blocks of RAM. One does this
// single-threaded, the other multi-threaded.

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <time.h>

#include "test/klib/memory/memory_tests.h"
#include "klib/memory/memory.h"
#include "test/test_core/test.h"

using namespace std;

namespace
{
  struct allocation
  {
    void *ptr;
    unsigned long size;
  };

  typedef std::vector<allocation> allocation_list;

  const unsigned long MAX_ALLOCATIONS = 1000;
  const unsigned long MAX_SINGLE_CHUNK = 262144;
  const unsigned long ITERATIONS = 1000000;

  const unsigned long NUM_THREADS = 2;

  pthread_t test_threads[NUM_THREADS];
}

void *memory_test_fuzz_allocation_thread(void *);

void memory_test_2()
{
  test_only_reset_allocator();

  memory_test_fuzz_allocation_thread(nullptr);
}

void memory_test_3_mt_fuzz()
{
  test_only_reset_allocator();

  // Ensure that the allocator is initialized before starting the test. This prevents both threads attempting to
  // initialize it at the same time. The allocator doesn't protect against this because it is guaranteed to be
  // initialized before multi-tasking begins in the kernel.
  void *temp = kmalloc(8);
  kfree(temp);

  srand (time(nullptr));

  for (int i = 0; i < NUM_THREADS; i++)
  {
    pthread_create(&test_threads[i], nullptr, &memory_test_fuzz_allocation_thread, nullptr);
  }

  for (int i = 0; i < NUM_THREADS; i++)
  {
    pthread_join(test_threads[i], nullptr);
  }
}

void *memory_test_fuzz_allocation_thread(void *)
{
  bool allocate = false;
  float proportion;
  unsigned long bytes_to_allocate;
  unsigned long dealloc_idx;
  allocation this_allocation;
  unsigned long allocation_count = 0;
  allocation_list completed_allocations;

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
      if (bytes_to_allocate > MAX_SINGLE_CHUNK)
      {
        bytes_to_allocate = MAX_SINGLE_CHUNK;
      }
      if (bytes_to_allocate == 0)
      {
        bytes_to_allocate = 1;
      }

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

  return nullptr;
}
