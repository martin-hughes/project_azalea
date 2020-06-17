/// @file
/// @brief Azalea online test program.
///
/// This suite is tests that can't be, or are difficult to, run without the live kernel.

#include "gtest/gtest.h"

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
