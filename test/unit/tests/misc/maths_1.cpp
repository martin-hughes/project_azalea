#include <iostream>

#include "math_hacks.h"
#include "test_core/test.h"

#include "gtest/gtest.h"

using namespace std;

struct pow2_test
{
  uint64_t input;
  uint64_t expected_output;
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

pow2_test log_base_two_tests[] = {
  { 0, 0 },
  { 1, 0 },
  { 2, 1 },
  { 3, 1 },
  { 4, 2 },
  { 5, 2 },
  { 7, 2 },
  { 8, 3 },
  { 31, 4 },
  { 32, 5 },
};

const uint64_t num_rounding_tests = sizeof (rounding_tests) / sizeof (pow2_test);
const uint64_t num_log_tests = sizeof(log_base_two_tests) / sizeof(pow2_test);

class MathsTest : public ::testing::Test
{
protected:
  MathsTest()
  {
  }

  virtual ~MathsTest()
  {
  }
};

// Create a new list, add and delete items, check the list is still valid.
TEST(MathsTest, RoundToNextPowerTwo)
{
  uint64_t actual_output;

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
      ASSERT_EQ(actual_output, rounding_tests[i].expected_output);
    }
  }
}

TEST(MathsTest, FindPowerOfTwo)
{
  uint64_t output;
  for (int i = 0; i < num_log_tests; i++)
  {
    output = which_power_of_two(log_base_two_tests[i].input);
    ASSERT_EQ(output, log_base_two_tests[i].expected_output);
  }
}
