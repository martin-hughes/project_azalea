#define ENABLE_TRACING

#include "klib/data_structures/red_black_tree.h"
#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <set>

using namespace std;

const unsigned long NUM_TESTS = 10000;
static set<unsigned long> keys;

void data_structures_test_3()
{
  cout << "Red black trees test" << endl;

  kl_rb_tree<unsigned long, unsigned long> *tree = new kl_rb_tree<unsigned long, unsigned long>();
  unsigned long cur_key;

  srand (time(nullptr));

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
    set<unsigned long>::iterator iter(keys.begin());
    for (int i = 0; i < (rand() % keys.size()); i++)
    {
      iter++;
    }

    cur_key = *iter;
    keys.erase(cur_key);
    ASSERT(tree->contains(cur_key));
    tree->remove(cur_key);
    tree->debug_verify_tree();
  }
}
