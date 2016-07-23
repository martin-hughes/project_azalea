// Dummy version of the klib-panic library, for use by the test code. Rather
// than aborting the whole system, raise an assertion - this can be picked up
// by the test code and failures can be handled gracefully.

#include "klib/tracing/tracing.h"
#include "test/test_core/test.h"

#include <iostream>
using namespace std;

void kl_trc_init_tracing()
{
  // TODO: Implement more interesting tracing.
}

void kl_trc_trace(unsigned long level, const char *message)
{
  cout << "TRC: (" << level << ") " << message << endl;
}
