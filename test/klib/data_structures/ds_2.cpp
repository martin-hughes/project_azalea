#include "test/test_core/test.h"
#include "klib/data_structures/binary_tree.h"
#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <set>
#include "gtest/gtest.h"

using namespace std;

const uint64_t NUM_TESTS = 100;
static set<uint64_t> keys;

TEST(DataStructuresTest, BinaryTrees1)
{
  cout << "Data structures test" << endl;

  kl_binary_tree<uint64_t, uint64_t> *tree = new kl_binary_tree<uint64_t, uint64_t>();
  uint64_t cur_key;

  srand (time(nullptr));

  ASSERT(!tree->contains(42));

  while(keys.size() < NUM_TESTS)
  {
    cur_key = rand() % 10000;
    auto return_val = keys.insert(cur_key);
    if (return_val.second)
    {
      //cout << keys.size() << ". Inserting key " << cur_key << "... " << flush;
      tree->insert(cur_key, 6);
      tree->debug_verify_tree();
      //cout << "Tree verified." << endl;
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
    //cout << keys.size() << ". Removing key " << cur_key << "... " << flush;
    keys.erase(cur_key);
    ASSERT(tree->contains(cur_key));
    tree->remove(cur_key);
    tree->debug_verify_tree();
    //cout << "Tree verfied." << endl;
  }

  delete tree;
}
