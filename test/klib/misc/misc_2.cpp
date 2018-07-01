#include "klib/c_helpers/buffers.h"
#include "test/test_core/test.h"

#include "gtest/gtest.h"

#include <iostream>
using namespace std;

TEST(KlibMiscTests, memcmp)
{
  char buffer_a[] = "1234";
  char buffer_b[] = "1235";

  ASSERT_EQ(kl_memcmp(buffer_a, buffer_b, 3), 0);
  ASSERT_EQ(kl_memcmp(buffer_a, buffer_b, 4), -1);
  ASSERT_EQ(kl_memcmp(buffer_a, buffer_a, 4), 0);
  ASSERT_EQ(kl_memcmp(buffer_b, buffer_a, 4), 1);
  ASSERT_EQ(kl_memcmp("a", "b", 0), 0);
}
