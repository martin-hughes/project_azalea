#include "gtest/gtest.h"
#include "system_tree_tests.h"

class SystemTreeTest : public ::testing::Test
{
protected:
};

TEST(SystemTreeTest, SimpleBranches)
{
  system_tree_test_1();
}

TEST(SystemTreeTest, SimpleTree)
{
  system_tree_test_2();
}

TEST(SystemTreeTest, GeneralPipes)
{
  system_tree_test_3_pipes();
}
