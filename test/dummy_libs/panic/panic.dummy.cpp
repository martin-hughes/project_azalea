// Dummy version of the klib-panic library, for use by the test code. Rather
// than aborting the whole system, raise an assertion - this can be picked up
// by the test code and failures can be handled gracefully.

#include "klib/panic/panic.h"
#include "test/test_core/test.h"
#include "gtest/gtest.h"

void panic(const char *message)
{
  // Cause Google Test to print out the assertion message by doing ADD_FAILURE(). However, this allows the test to
  // continue after this point, which is undesirable, so then throw a C++ assertion. Google Test will catch it and end
  // the current test.
  ADD_FAILURE() << message;
  throw new assertion_failure(message);
}
