// Kernel's main timing system.

#include "processor/timing/timing.h"
#include "processor/timing/timing-int.h"
#include "klib/klib.h"

// Initializes the kernel's timing systems. Currently makes the following assumptions:
// - ACPI is available on this system and is initialized.
// - At least one HPET is available, and can be found in the ACPI tables.
//
// This function will cause the HPET to start operating, and disable interrupts from the RTC and PIT.
//
// There is scope for emulating the high-precision element of the HPET using the PIT, processor cycle counting and so
// on, but that's a project for another time (and maybe never, what PC wouldn't have a HPET nowadays?)
void time_gen_init()
{
  KL_TRC_ENTRY;

  ASSERT(time_hpet_exists());

  time_hpet_init();

  KL_TRC_EXIT;
}

// Sleep the current process for the specified period.
void time_sleep_process(unsigned long wait_in_ns)
{
  INCOMPLETE_CODE(time_sleep_process);
}

void time_stall_process(unsigned long wait_in_ns)
{
  KL_TRC_ENTRY;

  KL_TRC_DATA("Stall for ns", wait_in_ns);
  time_hpet_stall(wait_in_ns);

  KL_TRC_EXIT;
}
