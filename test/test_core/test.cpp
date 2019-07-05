//------------------------------------------------------------------------------
// This is the entry point to all the standard tests. It provides a universal
// interface and features common to most tests.
//------------------------------------------------------------------------------

#include "test.h"

#include <iostream>
#include <chrono>
#include <string>

#include "gtest/gtest.h"
#ifdef UT_MEM_LEAK_CHECK
#include "win_mem_leak.h"
#endif

using namespace std;

global_test_opts_struct global_test_opts
{
  false
};

int main(int argc, char **argv)
{
  bool all_args_ok = true;

  // Initialise a shared data structure that doesn't need to always be reset.
  test_init_proc_interrupt_table();

  ::testing::InitGoogleTest(&argc, argv);

#ifdef UT_MEM_LEAK_CHECK
  ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new MemLeakListener);
#endif

  for (int i = 1; i < argc; i++)
  {
    string arg = argv[i];

    if (arg == "--keep-temp-files")
    {
      cout << "-- Will keep temporary files." << endl;
      global_test_opts.keep_temp_files = true;
    }
    else
    {
      cout << "Unrecognised argument: " << arg << endl;
      all_args_ok = false;
    }
  }

  if (!all_args_ok)
  {
    return 1;
  }
  else
  {
    return RUN_ALL_TESTS();
  }
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

std::ostream& operator<<(std::ostream& os, const ERR_CODE_T &ec)
{
  return os << "Error code: " << azalea_lookup_err_code(ec);
}
