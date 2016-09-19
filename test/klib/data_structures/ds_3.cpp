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

//const unsigned long known_insert_keys[] = {5967, 4703, 2889, 8954, 9724, 7427, 2273, 9707, 1388, 9834, 4112, 4028, 9902, 381, 9030, 6484, 4220, 3375, 4312, 4758};
//const unsigned long known_delete_keys[] = {4758, 4028, 3375, 4220, 2273, 4112, 6484, 1388, 7427, 381, 4312, 4703, 5967, 9030, 9724, };

//const unsigned long known_insert_keys[] = {9221, 291, 2548, 5076, 5648, 5425, 9986, 3198, 6007, 7264 };
//const unsigned long known_delete_keys[] = {5076, 3198, 291, 5648, 7264, 6007, 5425, };

//const unsigned long known_insert_keys[] = {5341, 337, 670, 7136, 8384, 1973, 8566, 4401, 441, 4524};
//const unsigned long known_delete_keys[] = {670, 337, 4524, 4401, 7136, };

//const unsigned long known_insert_keys[] = {4851, 2730, 6414, 8101, 3373, 9257, 2040, 4030, 9110, 3521, 3564, 5919, 2000, 5630, 1934, 6094, 8909, 2579, 5253, 944};
//const unsigned long known_delete_keys[] = {2040, 3373, 1934, 3564, 2730, 944, 4030, 4851, 5919, 5630, 6094, 6414,};

const unsigned long known_insert_keys[] = {2033, 4859, 3205, 294, 6901, 9489, 5131, 4912, 3472, 2348, 1941, 7435, 447, 8983, 7609, 6593, 5941, 3800, 1025, 3386};
const unsigned long known_delete_keys[] = {1025, 1941, 2033, 3386, 4859, 3800, 447, 3472, };

void data_structures_test_3()
{
  cout << "Red black trees test" << endl;

  kl_rb_tree<unsigned long, unsigned long> *tree = new kl_rb_tree<unsigned long, unsigned long>();
  unsigned long cur_key;

  srand (time(nullptr));

  /*for (unsigned long k : known_insert_keys)
  {
    cout << "Inserting " << k << endl;
    tree->insert(k, 5);
    keys.insert(k);
  }

  tree->debug_verify_tree();*/

  while(keys.size() < NUM_TESTS)
  {
    cur_key = rand() % 1000000;
    auto return_val = keys.insert(cur_key);
    if (return_val.second)
    {
      cout << keys.size() << ". Inserting key " << cur_key << "... " << flush;
      tree->insert(cur_key, 6);
      cout << "Tree verified." << endl;
      tree->debug_verify_tree();
    }
  }
  //tree->debug_verify_tree();

  /*for (unsigned long k : known_delete_keys)
  {
    cout << "DELETE " << k << endl;
    keys.erase(k);
    tree->remove(k);
    tree->debug_verify_tree();
  }*/

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
