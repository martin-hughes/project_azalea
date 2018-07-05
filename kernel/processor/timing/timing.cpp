// Kernel's main timing system.

#include "processor/timing/timing.h"
#include "processor/timing/timing-int.h"
#include "klib/klib.h"

/// @brief Initializes the kernel's timing systems.
///
/// Currently makes the following assumptions:
/// - ACPI is available on this system and is initialized.
/// - At least one HPET is available, and can be found in the ACPI tables.
///
/// This function will cause the HPET to start operating, and disable interrupts from the RTC and PIT.
///
/// There is scope for emulating the high-precision element of the HPET using the PIT, processor cycle counting and so
/// on, but that's a project for another time (and maybe never, what PC wouldn't have a HPET nowadays?)
void time_gen_init()
{
  KL_TRC_ENTRY;

  ASSERT(time_hpet_exists());

  time_hpet_init();

  KL_TRC_EXIT;
}

/// @brief Sleep the current process for the specified period.
///
/// Allows other processes to take over on this processor.
///
/// @param wait_in_ns The number of nanoseconds to sleep the current process for.
void time_sleep_process(uint64_t wait_in_ns)
{
  INCOMPLETE_CODE(time_sleep_process);
}

/// @brief Stall the process for the specified period.
///
/// Keeps running this process in a tight loop, but doesn't do anything to prevent the normal operation of the
/// scheduler!
///
/// @param wait_in_ns The number of nanoseconds to stall for.
void time_stall_process(uint64_t wait_in_ns)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Stall for ns", wait_in_ns, "\n");
  time_hpet_stall(wait_in_ns);

  KL_TRC_EXIT;
}

/// @brief Get the raw data from the system timer.
///
/// Returns the value of the HPET counter, for applications that may be interested - for example, for waiting a short
/// period whilst polling, or for performance measurements
///
/// @return The value of the system timer - the HPET in Azalea. May not be directly meaningful!
uint64_t time_get_system_timer_count()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return time_hpet_cur_value();
}

/// @brief Translate a desired wait into a number of system timer units.
///
/// Translate a desired waiting time into a value that can be used in conjunction with time_get_system_timer_count() to
/// check whether the desired wait has passed or not.
///
/// @param wait_in_ns The number of nanoseconds to translate into a system-dependent value.
///
/// @return A value to be added to time_get_system_timer_count() to check for the passing of time.
uint64_t time_get_system_timer_offset(uint64_t wait_in_ns)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return time_hpet_compute_wait(wait_in_ns);
}
