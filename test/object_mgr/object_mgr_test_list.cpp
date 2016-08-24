#include "test/object_mgr/object_mgr_test_list.h"
#include "test/test_core/test.h"

single_test test_list[] = {
    { object_mgr_test_1, "Handle management tests", },
    { object_mgr_test_2, "Simple object management tests", },
};
unsigned int number_of_tests = 2;
