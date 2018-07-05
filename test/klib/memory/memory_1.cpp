// Klib-memory test script 1.
//
// Simple allocation tests, testing various sizes of allocation and free.
// More complex tests to prove that the allocator works as expected are covered
// by later tests.

#include "test/klib/memory/memory_tests.h"
#include <iostream>

#include "klib/memory/memory.h"
#include "test/test_core/test.h"

using namespace std;

namespace
{
  const unsigned int PASSES = 5;
  const unsigned int SIZES_TO_TRY[] = {4, 8, 9, 63, 64, 65, 255, 1023, 262144};
  const unsigned int NUM_SIZES = sizeof(SIZES_TO_TRY) / sizeof(unsigned int);
}

void memory_test_1_try_size(unsigned int size);

void memory_test_1()
{
  void *result;
  cout << "Memory test 1" << endl;

  for (int i = 0; i < NUM_SIZES; i++)
  {
    memory_test_1_try_size(SIZES_TO_TRY[i]);
  }

  test_only_reset_allocator();
}

void memory_test_1_try_size(unsigned int size)
{
  test_only_reset_allocator();

  cout << "Testing size: " << size << endl;

  void *result;
  unsigned long result_store[PASSES];
  unsigned long last_diff = 0;

  // Allocate the correct number of chunks.
  for (unsigned int i = 0; i < PASSES; i++)
  {
    result = kmalloc(8);
    result_store[i] = (unsigned long)result;
  }

  // Confirm that the chunks are spaced as expected.
  last_diff = result_store[1] - result_store[0];
  for (unsigned int i = 1; i < PASSES; i++)
  {
    ASSERT(last_diff == (result_store[i] - result_store[i - 1]));
  }

  // Deallocate all of the results
  for (unsigned int i = 0; i < PASSES; i++)
  {
    kfree((void *)result_store[i]);
  }
}
