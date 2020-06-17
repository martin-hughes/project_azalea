/// @file
/// @brief Azalea system calls - timing interfaces.

//#define ENABLE_TRACING

#include "kernel_all.h"
#include "syscall_kernel-int.h"

/// @brief Return the current time in the system clock.
///
/// @param[out] buffer Storage for the current time.
///
/// @return ERR_CODE::INVALID_PARAM if buffer is not a suitable pointer, ERR_CODE::DEVICE_FAILED if the time couldn't
///         be successfully retrieved. ERR_CODE::NO_ERROR otherwise.
ERR_CODE az_get_system_clock(time_expanded *buffer)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  bool r;

  KL_TRC_ENTRY;

  if ((buffer == nullptr) || !SYSCALL_IS_UM_ADDRESS(buffer))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid buffer pointer\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    r = time_get_current_time(*buffer);
    if (!r)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to get time\n");
      result = ERR_CODE::DEVICE_FAILED;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Sleeps the current thread for the required time.
///
/// @param nanoseconds The thread will sleep for at least this many nanoseconds.
///
/// @return Always returns ERR_CODE::NO_ERROR.
ERR_CODE az_sleep_thread(uint64_t nanoseconds)
{
  KL_TRC_ENTRY;

  time_sleep_process(nanoseconds);

  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}
