#include "gtest/gtest.h"
#include "ds_tests.h"

class DataStructuresTest : public ::testing::Test
{
protected:
  DataStructuresTest()
  {
  }

  virtual ~DataStructuresTest()
  {
  }
};

TEST(DataStructuresTest, Lists1)
{
  data_structures_test_1();
}

TEST(DataStructuresTest, BinaryTrees1)
{
  data_structures_test_2();
}

TEST(DataStructuresTest, RedBlackTrees1)
{
  data_structures_test_3();
}
