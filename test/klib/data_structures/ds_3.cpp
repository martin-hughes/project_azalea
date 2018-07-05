#define ENABLE_TRACING

#include "klib/data_structures/red_black_tree.h"
#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <set>
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
