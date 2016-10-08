#include "gtest/gtest.h"
#include "memory_tests.h"

class KlibMemoryTest : public ::testing::Test
{
protected:
};

TEST(KlibMemoryTest, BasicTests)
{
  memory_test_1();
}

TEST(KlibMemoryTest, FuzzTests)
{
  memory_test_2();
}
