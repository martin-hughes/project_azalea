//------------------------------------------------------------------------------
// This is the entry point to all the standard tests. It provides a universal
// interface and features common to most tests.
//------------------------------------------------------------------------------

#include <iostream>
#include <chrono>
#include "test.h"

#include "gtest/gtest.h"

using namespace std;

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

assertion_failure::assertion_failure(const char *reason)
{
  _reason = reason;
}

const char *assertion_failure::get_reason()
{
  return _reason;
}

void test_spin_sleep(uint64_t sleep_time_ns)
{
  chrono::nanoseconds sleep_time(sleep_time_ns);
  auto start = chrono::high_resolution_clock::now();

  while((chrono::high_resolution_clock::now() - start) < sleep_time)
  {
    //spin
  }
}