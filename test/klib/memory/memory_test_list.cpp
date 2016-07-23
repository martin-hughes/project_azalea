#include "memory_test_list.h"
#include "test/test_core/test.h"

single_test test_list[] = {
    { memory_test_1, "Memory allocation test" },
    { memory_test_2, "Memory allocator fuzz" },
};
unsigned int number_of_tests = 2;
