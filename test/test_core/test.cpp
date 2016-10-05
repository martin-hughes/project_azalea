//------------------------------------------------------------------------------
// This is the entry point to all the standard tests. It provides a universal
// interface and features common to most tests.
//------------------------------------------------------------------------------

// On WIN32 builds of the tests, it is possible to do built-in memory leak detection.
#if defined(WIN32) || defined(_WIN64)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <iostream>
#include "test.h"

using namespace std;

int main()
{
  const char *test_name;
  test_entry_ptr test_fn;
  bool passed;
  bool all_passed = true;

#if defined(WIN32) || defined(_WIN64)
  _CrtMemState before_state;
  _CrtMemState after_state;
  _CrtMemState differences;
#endif

  for(unsigned int i=0; i < number_of_tests; i++)
  {
    test_name = test_list[i].test_name;
    test_fn = test_list[i].entry_point;
    cout << "Executing test #" << i + 1 << ": " << test_name << endl;
    passed = true;

#if defined(WIN32) || defined(_WIN64)
    _CrtMemCheckpoint(&before_state);
#endif

    try
    {
      test_fn();
    }
    catch(assertion_failure *asf)
    {
      cout << "Test FAILED: " << asf->get_reason() << endl;
      passed = false;
    }

#if defined(WIN32) || defined(_WIN64)
    _CrtMemCheckpoint(&after_state);
    if (_CrtMemDifference(&differences, &before_state, &after_state))
    {
      cout << "Test FAILED - MEMORY LEAK" << endl;
      _CrtMemDumpStatistics(&differences);
      passed = false;
    }
#endif

    if (passed)
    {
      cout << "Test PASSED" << endl;
    }
    else
    {
      all_passed = false;
    }
  }

#if defined(WIN32) || defined(_WIN64)
  _CrtDumpMemoryLeaks();
#endif

  return (all_passed ? 0 : 1);
}

assertion_failure::assertion_failure(const char *reason)
{
  _reason = reason;
}

const char *assertion_failure::get_reason()
{
  return _reason;
}
