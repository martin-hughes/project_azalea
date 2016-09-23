#include "test/klib/misc/misc_test_list.h"
#include "klib/c_helpers/string_fns.h"

#include "test/test_core/test.h"

// Create a new list, add and delete items, check the list is still valid.
void misc_test_1()
{
  ASSERT(kl_strlen("hello!", 0) == 6);
  ASSERT(kl_strlen("hello!", 4) == 4);
  ASSERT(kl_strlen("hello!", 8) == 6);
}
