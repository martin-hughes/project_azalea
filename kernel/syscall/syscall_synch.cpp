/// @file
/// @brief Synchronization primitives part of the system call interface - apart from futexes.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "object_mgr/object_mgr.h"
#include "processor/processor.h"

/// @brief Wait for an object before allowing this thread to continue.
///
/// This function will not return until the object being waited on signals that this thread can continue.
///
/// @param wait_object_handle Handle for an object that can be waited on.
///
/// @param max_wait The approximate maximum number of milliseconds to wait for this object. The wait time may be more
///                 due to having to wait for this thread to be scheduled again once the object has completed its wait.
///
/// @return ERR_CODE::NOT_FOUND if the handle does not correlate to any object. ERR_CODE::INVALID_OP if the object
///         cannot be waited on. ERR_CODE::NO_ERROR if the object was successfully waited on and then signalled us to
///         continue.
extern "C" ERR_CODE syscall_wait_for_object(GEN_HANDLE wait_object_handle, uint64_t max_wait)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  std::shared_ptr<WaitObject> wait_obj;
  task_thread *cur_thread = task_get_cur_thread();

  KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt to wait for handle: ", wait_object_handle, " for ", max_wait, "ms\n");

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    wait_obj = std::dynamic_pointer_cast<WaitObject>(
      cur_thread->parent_process->proc_handles.retrieve_handled_object(wait_object_handle));
    if (wait_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not a wait object\n");
      result = ERR_CODE::INVALID_OP;
    }
    else
    {
      if (max_wait < (SC_MAX_WAIT >> 10))
      {
        // The multiplication is to take milliseconds from the syscall interface to microseconds internally.
        max_wait *= 1000;
      }
      if (wait_obj->wait_for_signal(max_wait))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Wait and no timeout\n");
        result = ERR_CODE::NO_ERROR;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Wait had timeout\n");
        result = ERR_CODE::TIMED_OUT;
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Create a new mutex object
///
/// Mutexes can be acquired using syscall_wait_for_object(). They are released using syscall_release_mutex(). They are
/// destroyed using syscall_close_handle() - if a mutex is owned by the thread closing its handle, it is released.
///
/// @param[out] mutex_handle Space to store the newly created mutex's handle in.
///
/// @return A suitable error code.
ERR_CODE syscall_create_mutex(GEN_HANDLE *mutex_handle)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  task_thread *cur_thread = task_get_cur_thread();

  KL_TRC_ENTRY;

  if ((mutex_handle == nullptr) || (!SYSCALL_IS_UM_ADDRESS(mutex_handle)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Handle parameter invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  // All checks out, try to grab a system_tree object and allocate it a handle.
  else
  {
    object_data new_object;
    std::shared_ptr<syscall_mutex_obj> mut = std::make_shared<syscall_mutex_obj>();

    new_object.object_ptr = mut;
    *mutex_handle = cur_thread->parent_process->proc_handles.store_object(new_object);

    // Conceivably a user mode process could quickly change the handle in between the last line and the next one, but
    // it has no significant impact if they do.
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Correlated ", mut.get(), " to handle ", *mutex_handle, "\n");
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Release a mutex previously acquired using syscall_wait_for_object().
///
/// @param mutex_handle The mutex to release.
///
/// @return A suitable error code.
ERR_CODE syscall_release_mutex(GEN_HANDLE mutex_handle)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  task_thread *cur_thread = task_get_cur_thread();
  std::shared_ptr<IHandledObject> obj;
  std::shared_ptr<syscall_mutex_obj> mut;

  KL_TRC_ENTRY;

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    obj = cur_thread->parent_process->proc_handles.retrieve_handled_object(mutex_handle);
    if (obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Object not found!\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      mut = std::dynamic_pointer_cast<syscall_mutex_obj>(obj);
      if (mut)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found mutex\n");
        if (mut->release())
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully released\n");
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Not owned\n");
          result = ERR_CODE::INVALID_OP;
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Not a mutex\n");
        result = ERR_CODE::NOT_FOUND;
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Create a new semaphore object
///
/// Semaphores can be waited for using syscall_wait_for_object(). Waiting threads are signalled to continue using
/// syscall_signal_semaphore(). Handles are released by calling syscall_close_handle(), but this does not signal other
/// threads waiting for the semaphore.
///
/// @param[out] semaphore_handle Space to store the newly created semaphore's handle in.
///
/// @param max_users The maximum number of threads that can hold the semaphore at once.
///
/// @param start_users How many users should the semaphore consider itself to be held by at the start?
///
/// @return A suitable error code.
ERR_CODE syscall_create_semaphore(GEN_HANDLE *semaphore_handle, uint64_t max_users, uint64_t start_users)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  task_thread *cur_thread = task_get_cur_thread();

  KL_TRC_ENTRY;

  if ((semaphore_handle == nullptr) || (!SYSCALL_IS_UM_ADDRESS(semaphore_handle)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Handle parameter invalid\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  // All checks out, try to grab a system_tree object and allocate it a handle.
  else
  {
    object_data new_object;
    std::shared_ptr<syscall_semaphore_obj> sem = std::make_shared<syscall_semaphore_obj>(max_users, start_users);

    new_object.object_ptr = sem;
    *semaphore_handle = cur_thread->parent_process->proc_handles.store_object(new_object);

    // Conceivably a user mode process could quickly change the handle in between the last line and the next one, but
    // it has no significant impact if they do.
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Correlated ", sem.get(), " to handle ", *semaphore_handle, "\n");
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Signal the next thread waiting for this semaphore to continue.
///
/// @param semaphore_handle The handle of the semaphore to signal.
///
/// @return A suitable error code.
ERR_CODE syscall_signal_semaphore(GEN_HANDLE semaphore_handle)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  task_thread *cur_thread = task_get_cur_thread();
  std::shared_ptr<IHandledObject> obj;
  std::shared_ptr<syscall_semaphore_obj> sem;

  KL_TRC_ENTRY;

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    obj = cur_thread->parent_process->proc_handles.retrieve_handled_object(semaphore_handle);
    if (obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Object not found!\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      sem = std::dynamic_pointer_cast<syscall_semaphore_obj>(obj);
      if (sem)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found semaphore\n");
        if (sem->signal())
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Signalled successfully\n");
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't signal - bounds reached\n");
          result = ERR_CODE::INVALID_OP;
        }
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Not a semaphore\n");
        result = ERR_CODE::NOT_FOUND;
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void syscall_yield()
{
  KL_TRC_ENTRY;

  task_yield();

  KL_TRC_EXIT;
}
