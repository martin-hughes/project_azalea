/// @file
/// @brief Synchronization primitives part of the system call interface.
//
// Known defects:
// - All memory is currently RW, but when RO mappings become a thing we need to check for it before doing futex ops on
//   it.
// - This file uses task_thread * so what happens if the thread is deleted in the meantime?
// - The 'robust futex' concept isn't included yet, so what happens if a physical address is unused?
// - Is the way I retrieve the futex value and then wait a while really the correct way?

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
  ERR_CODE result;
  uint64_t phys_addr;
  int32_t cur_value;

  KL_TRC_ENTRY;

  futex_maybe_init();

  if (!SYSCALL_IS_UM_ADDRESS(const_cast<int32_t *>(futex)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid futex address\n");
    result = ERR_CODE::INVALID_PARAM;
  }

  cur_value = *futex;
  void *futex_v = reinterpret_cast<void *>(const_cast<int32_t *>(futex));
  phys_addr = reinterpret_cast<uint64_t>(mem_get_phys_addr(futex_v));

  if (!phys_addr)
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
      result = futex_wait(phys_addr, cur_value, req_value);
      break;

    case FUTEX_OP::FUTEX_WAKE:
      result = futex_wake(phys_addr);
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
