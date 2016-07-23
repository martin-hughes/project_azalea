// Dummy version of the klib-panic library, for use by the test code. Rather
// than aborting the whole system, raise an assertion - this can be picked up
// by the test code and failures can be handled gracefully.

#include "klib/panic/panic.h"
#include "test/test_core/test.h"

void panic(const char *message)
{
  throw new assertion_failure(message);
}
