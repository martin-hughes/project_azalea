#include "test_core/test.h"
#include "handles.h"

#include "gtest/gtest.h"

// A very simple test of the handle manager.

TEST(ObjectManagerTest, SimpleTests)
{
  GEN_HANDLE handle_a;
  GEN_HANDLE handle_b;

  handle_a = hm_get_handle();
  handle_b = hm_get_handle();

  ASSERT_NE(handle_a, handle_b);
  ASSERT_NE(handle_a, 0);
  ASSERT_NE(handle_b, 0);
}
