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
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  std::shared_ptr<WaitObject> wait_obj;
  task_thread *cur_thread = task_get_cur_thread();

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    wait_obj = std::dynamic_pointer_cast<WaitObject>(
      cur_thread->thread_handles.retrieve_handled_object(wait_object_handle));
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

/// @cond
// Since these system calls are not yet complete, don't document them yet.
ERR_CODE syscall_futex_wait(volatile int32_t *futex, int32_t req_value)
{
  INCOMPLETE_CODE("futex_wait");

  return ERR_CODE::UNKNOWN;
}

ERR_CODE syscall_futex_wake(volatile int32_t *futex)
{
  INCOMPLETE_CODE("futex_wake");

  return ERR_CODE::UNKNOWN;
}
/// @endcond
