#define ENABLE_TRACING

#include "test/test_core/test.h"
#include "klib/data_structures/red_black_tree.h"
#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <set>
#include <memory>
#include "gtest/gtest.h"

using namespace std;

const uint64_t NUM_TESTS = 10000;
static set<uint64_t> keys;

TEST(DataStructuresTest, RedBlackTrees1)
{
  cout << "Red black trees test" << endl;

  kl_rb_tree<uint64_t, uint64_t> *tree = new kl_rb_tree<uint64_t, uint64_t>();
  uint64_t cur_key;

  srand (time(nullptr));

  ASSERT(!tree->contains(65));

  while(keys.size() < NUM_TESTS)
  {
    cur_key = rand() % 1000000;
    auto return_val = keys.insert(cur_key);
    if (return_val.second)
    {
      tree->insert(cur_key, 6);
      tree->debug_verify_tree();
    }
  }

  while (keys.size() > 0)
  {
    set<uint64_t>::iterator iter(keys.begin());
    for (uint32_t i = 0; i < (rand() % keys.size()); i++)
    {
      iter++;
    }

    cur_key = *iter;
    keys.erase(cur_key);
    ASSERT(tree->contains(cur_key));
    tree->remove(cur_key);
    tree->debug_verify_tree();
  }

  delete tree;
}

// Ensures that we're not creating and losing copies of shared pointers.
TEST(DataStructuresTest, RedBlackTrees2)
{
  shared_ptr<int> a = make_shared<int>(5);
  shared_ptr<int> b;
  kl_rb_tree<int, shared_ptr<int>> tree;

  ASSERT_TRUE(a);
  ASSERT_EQ(*a, 5);
  ASSERT_EQ(a.use_count(), 1);

  tree.insert(1, a);
  ASSERT_EQ(a.use_count(), 2);

  b = tree.search(1);
  ASSERT_EQ(a.use_count(), 3);

  b = nullptr;
  ASSERT_EQ(a.use_count(), 2);

  tree.remove(1);
  ASSERT_EQ(a.use_count(), 1);
}