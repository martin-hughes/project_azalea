#include "klib/data_structures/binary_tree.h"
#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <set>

using namespace std;

const unsigned long NUM_TESTS = 100;
set<unsigned long> keys;

void data_structures_test_2()
{
  cout << "Data structures test" << endl;

  kl_binary_tree<unsigned long, unsigned long> *tree = new kl_binary_tree<unsigned long, unsigned long>();
  unsigned long cur_key;

  srand (time(nullptr));

  while(keys.size() < NUM_TESTS)
  {
    cur_key = rand() % 10000;
    auto return_val = keys.insert(cur_key);
    if (return_val.second)
    {
      cout << keys.size() << ". Inserting key " << cur_key << "... " << flush;
      tree->insert(cur_key, 6);
      tree->debug_verify_tree();
      cout << "Tree verified." << endl;
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
    cout << keys.size() << ". Removing key " << cur_key << "... " << flush;
    keys.erase(cur_key);
    ASSERT(tree->contains(cur_key));
    tree->remove(cur_key);
    tree->debug_verify_tree();
    cout << "Tree verfied." << endl;
  }
}
