#include "klib/c_helpers/string_fns.h"
#include "test/test_core/test.h"

#include "gtest/gtest.h"

TEST(KlibMiscTests, strlen)
{
  ASSERT_EQ(kl_strlen("hello!", 0), 0);
  ASSERT_EQ(kl_strlen("hello!", 4), 4);
  ASSERT_EQ(kl_strlen("hello!", 8), 6);
}
