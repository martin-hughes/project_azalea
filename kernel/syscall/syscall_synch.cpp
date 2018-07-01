/// @file
/// @brief Synchronization primitives part of the system call interface.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "object_mgr/object_mgr.h"

/// @brief Wait for an object before allowing this thread to continue.
///
/// This function will not return until the object being waited on signals that this thread can continue.
///
/// @param wait_object_handle Handle for an object that can be waited on.
///
/// @return ERR_CODE::NOT_FOUND if the handle does not correlate to any object. ERR_CODE::INVALID_OP if the object
///         cannot be waited on. ERR_CODE::NO_ERROR if the object was successfully waited on and then signalled us to
///         continue.
extern "C" ERR_CODE syscall_wait_for_object(GEN_HANDLE wait_object_handle)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  IRefCounted *om_obj;
  WaitObject *wait_obj;

  KL_TRC_ENTRY;

  om_obj = om_retrieve_object(wait_object_handle);
  if (om_obj == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not found on OM\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    wait_obj = dynamic_cast<WaitObject *>(om_obj);

    if (wait_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not a wait object\n");
      result = ERR_CODE::INVALID_OP;
    }
    else
    {
      wait_obj->wait_for_signal();
      result = ERR_CODE::NO_ERROR;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE syscall_futex_wait(volatile int *futex, int req_value)
{
  INCOMPLETE_CODE("futext_wait");

  return ERR_CODE::UNKNOWN;
}

ERR_CODE syscall_futex_wake(volatile int *futex)
{
  INCOMPLETE_CODE("futext_wake");

  return ERR_CODE::UNKNOWN;
}
