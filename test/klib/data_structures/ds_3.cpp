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

const unsigned long NUM_TESTS = 100;
static set<unsigned long> keys;

const unsigned long known_insert_keys[] = {5967, 4703, 2889, 8954, 9724, 7427, 2273, 9707, 1388, 9834, 4112, 4028, 9902, 381, 9030, 6484, 4220, 3375, 4312, 4758};
const unsigned long known_delete_keys[] = {4758, 4028, 3375, 4220, 2273, 4112, 6484, 1388, 7427, 381, 4312, 4703, 5967, 9030, 9724, };

void data_structures_test_3()
{
  cout << "Red black trees test" << endl;

  kl_rb_tree<unsigned long, unsigned long> *tree = new kl_rb_tree<unsigned long, unsigned long>();
  unsigned long cur_key;

  srand (time(nullptr));

  for (unsigned long k : known_insert_keys)
  {
    cout << "Inserting " << k << endl;
    tree->insert(k, 5);
    keys.insert(k);
  }

  tree->debug_verify_tree();

  /*while(keys.size() < NUM_TESTS)
  {
    cur_key = rand() % 10000;
    auto return_val = keys.insert(cur_key);
    if (return_val.second)
    {
      cout << keys.size() << ". Inserting key " << cur_key << "... " << flush;
      tree->insert(cur_key, 6);
      cout << "Tree verified." << endl;
      tree->debug_verify_tree();
    }
  }*/

  for (unsigned long k : known_delete_keys)
  {
    cout << "DELETE " << k << endl;
    keys.erase(k);
    tree->remove(k);
    tree->debug_verify_tree();
  }

  while (keys.size() > 0)
  {
    set<unsigned long>::iterator iter(keys.begin());
    for (int i = 0; i < (rand() % keys.size()); i++)
    {
      iter++;
    }

    cur_key = *iter;
    cout << keys.size() << ". Removing key " << cur_key << "... " << endl;
    keys.erase(cur_key);
    ASSERT(tree->contains(cur_key));
    tree->remove(cur_key);
    tree->debug_verify_tree();
    cout << "Tree verfied." << endl;
  }
}
