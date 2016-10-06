#include "gtest/gtest.h"
#include "object_mgr_tests.h"

class ObjectManagerTest : public ::testing::Test
{
protected:
};

TEST(ObjectManagerTest, SimpleTests)
{
  object_mgr_test_1();
}

TEST(ObjectManagerTest, StoreAndRetrieve)
{
  object_mgr_test_2();
}
