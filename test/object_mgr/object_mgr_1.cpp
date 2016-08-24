#include "object_mgr/handles.h"

#include "test/object_mgr/object_mgr_test_list.h"
#include "test/test_core/test.h"
#include <iostream>

using namespace std;

// A very simple test of the handle manager.
void object_mgr_test_1()
{
  GEN_HANDLE handle_a;
  GEN_HANDLE handle_b;

  cout << "Handle Manager - Simple tests" << endl;

  handle_a = hm_get_handle();
  handle_b = hm_get_handle();

  ASSERT(handle_a != handle_b);
  ASSERT(handle_a != 0);
  ASSERT(handle_b != 0);
}
