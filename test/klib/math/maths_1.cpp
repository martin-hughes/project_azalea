#include <iostream>

#include "klib/misc/math_hacks.h"
#include "maths_test_list.h"
#include "test/test_core/test.h"

using namespace std;

struct pow2_test
{
  unsigned long input;
  unsigned long expected_output;
};

pow2_test rounding_tests[] = {
    { 0, 0 },
    { 1, 1 },
    { 2, 2 },
    { 3, 4 },
    { 4, 4 },
    { 5, 8 },
    { 7, 8 },
    { 0x1000001, 0x2000000 },
    { 0x1000000000000001, 0 },
};

const unsigned long num_rounding_tests = sizeof (rounding_tests) / sizeof (pow2_test);


// Create a new list, add and delete items, check the list is still valid.
void maths_test_1()
{
  unsigned long actual_output;

  cout << "Math: round_to_power_two() tests" << endl;

  for (int i = 0; i < num_rounding_tests; i++)
  {
    actual_output = round_to_power_two(rounding_tests[i].input);
    cout << "Input: " << rounding_tests[i].input;
    cout << ", Output: " << actual_output;
    cout << ", Expected: " << rounding_tests[i].expected_output;
    if (actual_output == rounding_tests[i].expected_output)
    {
      cout << " OK" << endl;
    }
    else
    {
      cout << " FAIL" << endl;
      ASSERT(actual_output == rounding_tests[i].expected_output);
    }
  }
}
