#include <test/tracing/tracing_tests.h>
#include "gtest/gtest.h"

class TracingTest : public ::testing::Test
{
protected:
};

TEST(TracingTest, SimpleTracing)
{
  tracing_test_1();
}
