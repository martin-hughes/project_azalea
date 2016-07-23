//------------------------------------------------------------------------------
// This is the entry point to all the standard tests. It provides a universal
// interface and features common to most tests.
//------------------------------------------------------------------------------

#include <iostream>
#include "test.h"

using namespace std;

int main()
{
  const char *test_name;
  test_entry_ptr test_fn;
  bool passed;

  for(int i=0; i < number_of_tests; i++)
  {
    test_name = test_list[i].test_name;
    test_fn = test_list[i].entry_point;
    cout << "Executing test #" << i << ": " << test_name << endl;
    passed = true;
    try
    {
      test_fn();
    }
    catch(assertion_failure *asf)
    {
      cout << "Test FAILED: " << asf->get_reason() << endl;
      passed = false;
    }

    if (passed)
    {
      cout << "Test PASSED" << endl;
    }
  }
  return 0;
}

assertion_failure::assertion_failure(const char *reason)
{
  _reason = reason;
}

const char *assertion_failure::get_reason()
{
  return _reason;
}
