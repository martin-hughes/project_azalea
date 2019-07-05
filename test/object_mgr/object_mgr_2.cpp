#include "test/test_core/test.h"
#include "system_tree/system_tree_leaf.h"
#include "object_mgr/handles.h"
#include "object_mgr/object_mgr.h"
#include "processor/processor.h"

#include "gtest/gtest.h"

using namespace std;

const uint32_t NUM_OBJECTS = 5;

class simple_object : public ISystemTreeLeaf
{
public:
  virtual ~simple_object() {}
};

// A very simple test of the handle manager.
TEST(ObjectManagerTest, StoreAndRetrieve)
{
  shared_ptr<simple_object> objects[NUM_OBJECTS];
  GEN_HANDLE handles[NUM_OBJECTS];
  unique_ptr<object_manager> om = std::make_unique<object_manager>();

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    object_data d;
    objects[i] = make_shared<simple_object>();
    ASSERT_EQ(objects[i].use_count(), 1);
    d.object_ptr = dynamic_pointer_cast<IHandledObject>(objects[i]);
    handles[i] = om->store_object(d);
    d.object_ptr = nullptr;
    ASSERT_EQ(objects[i].use_count(), 2);
  }

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    ASSERT_EQ(objects[i], dynamic_pointer_cast<simple_object >(om->retrieve_object(handles[i])->object_ptr));
  }

  for (int i = 0; i < NUM_OBJECTS; i++)
  {
    om->remove_object(handles[i]);
    ASSERT_EQ(objects[i].use_count(), 1);
  }
}
