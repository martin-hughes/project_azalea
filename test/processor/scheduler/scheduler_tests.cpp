#include "gtest/gtest.h"
#include "scheduler_tests.h"

class SchedulerTest : public ::testing::Test
{
protected:
};

TEST(SchedulerTest, SimpleTests)
{
  scheduler_test_1();
}
