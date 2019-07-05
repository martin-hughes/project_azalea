// Dummy interface for timing functions when running in the test scripts

#include "processor/timing/timing.h"
#include "test/test_core/test.h"

void time_stall_process(uint64_t wait_in_ns)
{
  test_spin_sleep(wait_in_ns);
}

bool time_get_current_time(time_expanded &time)
{
  return false;
}