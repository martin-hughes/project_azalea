// Dummy interface for timing functions when running in the test scripts

#include "timing.h"
#include "test_core/test.h"

namespace
{
  uint64_t system_timer_count{0};
}

void time_stall_process(uint64_t wait_in_ns)
{
  test_spin_sleep(wait_in_ns);
}

bool time_get_current_time(time_expanded &time)
{
  return false;
}

uint64_t time_get_system_timer_count(bool output_in_ns)
{
  // output_in_ns is unusued.
  return system_timer_count;
}

void test_set_system_timer_count(uint64_t count)
{
  system_timer_count = count;
}