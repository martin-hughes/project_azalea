#include "synch_test_list.h"

#include "test/test_core/test.h"

single_test test_list[] = {
    { synch_test_1, "Straightforward spinlock lock/unlock tests" },
    { synch_test_2, "Straightforward mutex lock/unlock tests" },
};

unsigned int number_of_tests = 2;
