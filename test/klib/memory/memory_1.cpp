// Klib-memory test script 1.
//
// Simple allocation tests, testing various sizes of allocation and free.
// More complex tests to prove that the allocator works as expected are covered
// by later tests.

#include "test/test_core/test.h"

#include <iostream>
#include "gtest/gtest.h"

#include "klib/memory/memory.h"

using namespace std;

namespace
{
  const uint32_t PASSES = 5;
  const uint32_t SIZES_TO_TRY[] = {4, 8, 9, 63, 64, 65, 255, 1023, 262144};
  const uint32_t NUM_SIZES = sizeof(SIZES_TO_TRY) / sizeof(uint32_t);
}

void memory_test_1_try_size(uint32_t size);

TEST(KlibMemoryTest, BasicTests)
{
  cout << "Memory test 1" << endl;

  for (int i = 0; i < NUM_SIZES; i++)
  {
    memory_test_1_try_size(SIZES_TO_TRY[i]);
  }

  test_only_reset_allocator();
}

void memory_test_1_try_size(uint32_t size)
{
  test_only_reset_allocator();

  cout << "Testing size: " << size << endl;

  void *result = nullptr;
  uint64_t result_store[PASSES];
  uint64_t last_diff = 0;

  // Allocate the correct number of chunks.
  for (uint32_t i = 0; i < PASSES; i++)
  {
    result = kmalloc(size);
    result_store[i] = (uint64_t)result;
  }

  // Confirm that the chunks are spaced as expected.
  last_diff = result_store[1] - result_store[0];
  for (uint32_t i = 1; i < PASSES; i++)
  {
    ASSERT_EQ(last_diff, (result_store[i] - result_store[i - 1]));
  }

  // Deallocate all of the results
  for (uint32_t i = 0; i < PASSES; i++)
  {
    kfree((void *)result_store[i]);
  }
}
