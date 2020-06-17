/// @file
/// @brief Kernel's main timing system.

#include "timing.h"
#include "timing-int.h"
#include "processor.h"

#include <vector>

namespace
{
  /// @brief A set containing all the clocks known to be in the system.
  ///
  /// Ideally this would be implemented using std::set, but we don't have a complete kernel-mode C++ library yet.
  std::vector<std::shared_ptr<IGenericClock>> *clock_array{nullptr};

  ipc::raw_spinlock clock_array_lock; ///< Lock protecting clock_array.
}

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
///
/// This function is assumed to be called while still in single-threaded mode, so no locking is needed around global
/// variables
void time_gen_init()
{
  KL_TRC_ENTRY;

  clock_array = new std::vector<std::shared_ptr<IGenericClock>>();

  ipc_raw_spinlock_init(clock_array_lock);

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
  task_thread *t = task_get_cur_thread();
  KL_TRC_ENTRY;

  ASSERT(t);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Sleep thread ", t, " for ", wait_in_ns, " ns.\n");
  uint64_t cur_time = time_get_system_timer_count(true);
  t->wake_thread_after = cur_time + wait_in_ns;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Wake after time: ", t->wake_thread_after, "\n");
  t->permit_running = false;
  task_yield();

  KL_TRC_EXIT;
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
/// @param output_in_ns - If set to true, output the system timer count in terms of nanoseconds. If false, just output
///                       the raw value.
///
/// @return The value of the system timer - the HPET in Azalea. May not be directly meaningful!
uint64_t time_get_system_timer_count(bool output_in_ns)
{
  uint64_t val;
  KL_TRC_ENTRY;

  val =  time_hpet_cur_value(output_in_ns);

  KL_TRC_EXIT;

  return val;
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

/// @brief Add a clock device to the system's pool of time sources.
///
/// In the future, the system will endeavour to merge all sources of time to get the highest precision. For now, it
/// does not.
///
/// @param clock The clock source to add to the system timing pool.
///
/// @return True if this was successful, false otherwise.
bool time_register_clock_source(std::shared_ptr<IGenericClock> clock)
{
  bool result{true};

  KL_TRC_ENTRY;

  if (clock_array != nullptr)
  {
    ipc_raw_spinlock_lock(clock_array_lock);

    for (auto e : *clock_array)
    {
      if (e == clock)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Clock already registered!\n");
        result = false;
        break;
      }
    }

    if (result)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not already registered, so add now\n");
      clock_array->push_back(clock);
    }

    ipc_raw_spinlock_unlock(clock_array_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Remove a clock device from the system's pool of time sources.
///
/// @param clock The clock source to remove from the system timing pool.
///
/// @return True if this was successful, false otherwise.
bool time_unregister_clock_source(std::shared_ptr<IGenericClock> clock)
{
  bool result{false};

  KL_TRC_ENTRY;

  if (clock_array != nullptr)
  {
    ipc_raw_spinlock_lock(clock_array_lock);
    for (auto e = clock_array->begin(); e != clock_array->end(); e++)
    {
      if (*e == clock)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Clock found for removal!\n");
        result = true;
        clock_array->erase(e); // This invalidates the iterator, of course.
        break;
      }
    }
    ipc_raw_spinlock_unlock(clock_array_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Get the current time.
///
/// @param[out] time If it can be retrieved, stores the current system time.
///
/// @return true if the time was calculated OK and stored in time, false otherwise.
bool time_get_current_time(time_expanded &time)
{
  bool result{false};

  KL_TRC_ENTRY;

  if ((clock_array != nullptr) && (clock_array->size() > 0))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Get first clock to handle this...\n");
    result = (*clock_array->begin())->get_current_time(time);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
