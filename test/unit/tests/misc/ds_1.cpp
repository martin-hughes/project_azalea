#include "types/list.h"
#include <iostream>
#include "gtest/gtest.h"

#include "test_core/test.h"

using namespace std;

// Use these in the tests, it's simpler than allocating and destroying them.
const uint32_t num_demo_items = 5;
klib_list_item<uint32_t *> demo_items[num_demo_items];

// Create a new list, add and delete items, check the list is still valid.
TEST(DataStructuresTest, Lists1)
{
  cout << "Lists test 1" << endl;

  // Initialize the demo items list
  for (int i = 0; i < num_demo_items; i++)
  {
    klib_list_item_initialize(&demo_items[i]);
    demo_items[i].item = const_cast<uint32_t *>(&num_demo_items);
  }

  // Test the empty list.
  klib_list<uint32_t *> list_root;
  klib_list_initialize(&list_root);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(klib_list_is_empty(&list_root));

  // Try mushing the list object's pointers.
  list_root.head = &demo_items[0];
  ASSERT(!klib_list_is_valid(&list_root));
  list_root.head = nullptr;
  list_root.tail = &demo_items[0];
  ASSERT(!klib_list_is_valid(&list_root));
  list_root.tail = nullptr;

  // Add an item at the head and remove it again.
  klib_list_add_head(&list_root, &demo_items[0]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(!klib_list_is_empty(&list_root));
  klib_list_remove(&demo_items[0]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(klib_list_is_empty(&list_root));

  // Do the same at the tail.
  klib_list_add_tail(&list_root, &demo_items[0]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(!klib_list_is_empty(&list_root));
  klib_list_remove(&demo_items[0]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(klib_list_is_empty(&list_root));

  // Do a bit of chopping and changing.
  klib_list_add_head(&list_root, &demo_items[0]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(!klib_list_is_empty(&list_root));
  klib_list_add_head(&list_root, &demo_items[1]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(!klib_list_is_empty(&list_root));
  klib_list_add_tail(&list_root, &demo_items[2]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(!klib_list_is_empty(&list_root));
  klib_list_add_after(&demo_items[2], &demo_items[3]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(!klib_list_is_empty(&list_root));
  klib_list_add_before(&demo_items[1], &demo_items[4]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(!klib_list_is_empty(&list_root));
  klib_list_remove(&demo_items[3]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(!klib_list_is_empty(&list_root));
  klib_list_add_after(&demo_items[2], &demo_items[3]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(!klib_list_is_empty(&list_root));

  // Check the ordering of items in the list. No need to do it both ways, that
  // ought to have been done by klib_list_is_valid().
  ASSERT(list_root.head == &demo_items[4]);
  ASSERT(demo_items[4].next == &demo_items[1]);
  ASSERT(demo_items[1].next == &demo_items[0]);
  ASSERT(demo_items[0].next == &demo_items[2]);
  ASSERT(demo_items[2].next == &demo_items[3]);
  ASSERT(list_root.tail == &demo_items[3]);

  ASSERT(klib_list_get_length(&list_root) == 5);

  klib_list_remove(&demo_items[3]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(list_root.tail == &demo_items[2]);

  klib_list_remove(&demo_items[4]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(list_root.head == &demo_items[1]);

  klib_list_remove(&demo_items[0]);
  ASSERT(klib_list_is_valid(&list_root));

  klib_list_remove(&demo_items[2]);
  ASSERT(klib_list_is_valid(&list_root));
  klib_list_remove(&demo_items[1]);
  ASSERT(klib_list_is_valid(&list_root));
  ASSERT(klib_list_is_empty(&list_root));
}
