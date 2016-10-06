#include "gtest/gtest.h"
#include "synch_tests.h"

class KlibSynchTest : public ::testing::Test
{
protected:
};

TEST(KlibSynchTest, Lists1)
{
  synch_test_1();
}
