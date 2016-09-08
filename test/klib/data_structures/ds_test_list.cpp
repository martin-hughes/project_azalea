#include "test/klib/data_structures/ds_test_list.h"
#include "test/test_core/test.h"

single_test test_list[] = {
    { data_structures_test_1, "Basic list test" },
    { data_structures_test_2, "Basic BST test" },
    { data_structures_test_3, "Basic red-black tree test" },
};
unsigned int number_of_tests = 3;
