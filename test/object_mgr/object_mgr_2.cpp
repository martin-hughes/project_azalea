#include "test/object_mgr/object_mgr_tests.h"
#include "object_mgr/handles.h"
#include "object_mgr/object_mgr.h"
#include "test/test_core/test.h"
#include <iostream>

using namespace std;

const unsigned int NUM_OBJECTS = 5;

// A very simple test of the handle manager.
void object_mgr_test_2()
{
  int objects[NUM_OBJECTS];
  GEN_HANDLE handles[NUM_OBJECTS];

  cout << "Object Manager - Simple tests" << endl;

  om_gen_init();

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    handles[i] = om_store_object(&objects[i]);
  }

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    ASSERT(&objects[i] == om_retrieve_object(handles[i]));
  }

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    om_remove_object(handles[i]);
  }

  test_only_reset_om();
}
