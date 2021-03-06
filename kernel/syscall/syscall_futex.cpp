/// @file
/// @brief Synchronization primitives part of the system call interface.
//
// Known defects:
// - None at present

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "processor/processor.h"
#include "processor/futexes.h"

#include <map>

/// @brief Provides futexes in a similar manner to Linux
///
/// Full details of supported operations can be found in the Linux futex documentation. Note that Azalea does not
/// support all futex operations yet.
///
/// Use one syscall for all futex operations rather than one syscall per operation to match the occasionally-expanding
/// Linux interface - this will make it easier to keep up.
///
/// @param futex Address of the futex to operate on.
///
/// @param op Operation to carry out on futex.
///
/// @param req_value The requested value of futex to wait for, if applicable
///
/// @param timeout_ns The amount of time to wait for the operation to complete, in ns, if applicable. (Not currently
///                   supported)
///
/// @param futex_2 Where needed, the address of a second futex (not used in Azalea yet)
///
/// @param v3 Where needed, another value for the futex op (not used in Azalea yet)
///
/// @return A suitable error code. Examples include:
///
///         - ERR_CODE::NO_ERROR if the operation was successful.
///
///         - ERR_CODE::INVALID_OP if op wasn't recognised.
///
///         - ERR_CODE::INVALID_PARAM if one or more parameters didn't make sense
///
///         - ERR_CODE::NOT_FOUND if the futex didn't already exist but was supposed to.
ERR_CODE syscall_futex_op(volatile int32_t *futex,
                          FUTEX_OP op,
                          int32_t req_value,
                          uint64_t timeout_ns,
                          volatile int32_t *futex_2,
                          uint32_t v3)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  int32_t cur_value;

  KL_TRC_ENTRY;

  if (!SYSCALL_IS_UM_ADDRESS(const_cast<int32_t *>(futex)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid futex address\n");
    result = ERR_CODE::INVALID_PARAM;
  }

  cur_value = *futex;
  void *futex_v = reinterpret_cast<void *>(const_cast<int32_t *>(futex));

  if (!mem_get_phys_addr(futex_v))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not a mapped physical address\n");
    result = ERR_CODE::INVALID_PARAM;
  }

  if (result == ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No failures so far, attempt op\n");

    switch (op)
    {
    case FUTEX_OP::FUTEX_WAIT:
      result = futex_wait(futex, req_value);
      break;

    case FUTEX_OP::FUTEX_WAKE:
      result = futex_wake(futex);
      break;

    case FUTEX_OP::FUTEX_REQUEUE:
      INCOMPLETE_CODE("Requeue futex");

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown operation\n");
      result = ERR_CODE::INVALID_OP;
      break;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
