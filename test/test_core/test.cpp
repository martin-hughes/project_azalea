//------------------------------------------------------------------------------
// This is the entry point to all the standard tests. It provides a universal
// interface and features common to most tests.
//------------------------------------------------------------------------------

#include <iostream>
#include <chrono>
#include "test.h"

#include <iostream>
#include <chrono>

#include "gtest/gtest.h"
#ifdef UT_MEM_LEAK_CHECK
#include "win_mem_leak.h"
#endif

using namespace std;

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

#ifdef UT_MEM_LEAK_CHECK
  ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new MemLeakListener);
#endif

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
