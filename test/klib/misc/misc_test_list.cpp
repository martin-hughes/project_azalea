#include "test/klib/misc/misc_test_list.h"
#include "test/test_core/test.h"

single_test test_list[] = {
    { misc_test_1, "kl_strlen tests" },
    { misc_test_2, "kl_memcmp tests" },
};
unsigned int number_of_tests = 2;
