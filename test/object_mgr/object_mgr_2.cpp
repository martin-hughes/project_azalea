#include "test/test_core/test.h"
#include "system_tree/system_tree_leaf.h"
#include "object_mgr/handles.h"
#include "object_mgr/object_mgr.h"
#include "processor/processor.h"

#include "gtest/gtest.h"

const uint32_t NUM_OBJECTS = 5;

class simple_object : public ISystemTreeLeaf
{
public:
  virtual ~simple_object() {}
};

// A very simple test of the handle manager.
TEST(ObjectManagerTest, StoreAndRetrieve)
{
  simple_object objects[NUM_OBJECTS];
  GEN_HANDLE handles[NUM_OBJECTS];

  om_gen_init();

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    handles[i] = om_store_object(dynamic_cast<IRefCounted *>(&objects[i]));
  }

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    ASSERT_EQ(&objects[i], dynamic_cast<simple_object *>(om_retrieve_object(handles[i])));
  }

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    om_remove_object(handles[i]);
  }

  test_only_reset_om();
}

TEST(ObjectManagerTest, ThreadSpecificHandles)
{
  simple_object objects[NUM_OBJECTS];
  GEN_HANDLE handles[NUM_OBJECTS];
  task_thread threads[NUM_OBJECTS];
  int j;

  om_gen_init();

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    test_only_set_cur_thread(&threads[i]);
    handles[i] = om_store_object(dynamic_cast<IRefCounted *>(&objects[i]));
  }

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    // It should only be possible to retrieve objects when they are associated with the currently running thread.
    test_only_set_cur_thread(&threads[i]);
    ASSERT_EQ(&objects[i], dynamic_cast<simple_object *>(om_retrieve_object(handles[i])));

    j = (i == 0) ? NUM_OBJECTS - 1 : i - 1;
    test_only_set_cur_thread(&threads[j]);
    ASSERT_EQ(nullptr, dynamic_cast<simple_object *>(om_retrieve_object(handles[i])));
  }

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    test_only_set_cur_thread(&threads[i]);
    om_remove_object(handles[i]);
  }

  test_only_reset_om();
}
