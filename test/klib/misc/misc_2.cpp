#include "test/klib/misc/misc_test_list.h"
#include "klib/c_helpers/buffers.h"

#include "test/test_core/test.h"

#include <iostream>
using namespace std;

// Create a new list, add and delete items, check the list is still valid.
void misc_test_2()
{
  char buffer_a[] = "1234";
  char buffer_b[] = "1235";

  ASSERT(kl_memcmp(buffer_a, buffer_b, 3) == 0);
  ASSERT(kl_memcmp(buffer_a, buffer_b, 4) == -1);
  ASSERT(kl_memcmp(buffer_a, buffer_a, 4) == 0);
  ASSERT(kl_memcmp(buffer_b, buffer_a, 4) == 1);
  ASSERT(kl_memcmp("a", "b", 0) == 0);
}
