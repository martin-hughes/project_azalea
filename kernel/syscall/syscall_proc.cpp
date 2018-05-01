/// @file
/// @brief Process and thread control part of the system call interface

//#define ENABLE_TRACING

#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "object_mgr/object_mgr.h"
#include "klib/klib.h"

#include "processor/processor-int.h"

/// @brief Create a new process
///
/// Creates a new process. The new process contains one thread which will start at `entry_point_addr`. No memory is
/// mapped apart from a single stack for the initial thread, so it will be necessary to map and populate memory pages
/// before calling `syscall_start_process`.
///
/// @param entry_point_addr[in] The virtual memory starting address for the new process, in the context of the new
///                             process. As noted above, it is necessary to create the memory mappings to fulfil this
///                             entry point before calling `syscall_start_process`
///
/// @param proc_handle[out] Storage for a handle to the new process.
///
/// @return ERR_CODE::INVALID_PARAM if either parameter contains an invalid address. ERR_CODE::NO_ERROR otherwise.
ERR_CODE syscall_create_process(void *entry_point_addr, GEN_HANDLE *proc_handle)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  task_process *new_process;

  KL_TRC_ENTRY;

  if ((entry_point_addr == nullptr) ||
      (!SYSCALL_IS_UM_ADDRESS(entry_point_addr)) ||
      (proc_handle == nullptr) ||
      (!SYSCALL_IS_UM_ADDRESS(proc_handle)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parameters\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    new_process = task_create_new_process(reinterpret_cast<ENTRY_PROC>(entry_point_addr));

    if (new_process != nullptr)
    {
      *proc_handle = om_store_object(new_process);
      KL_TRC_TRACE(TRC_LVL::FLOW, "New process (", new_process, ") created, handle: ", *proc_handle, "\n");

      // The creation of the process has caused this code to be a reference holder of the process object, but we want
      // the only reference holder to be the handle - so release our reference now.
      new_process->ref_release();

      result = ERR_CODE::NO_ERROR;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to create process\n");
      result = ERR_CODE::INVALID_OP;
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Cause a process to start running.
///
/// Has no effect if the process is already running.
///
/// @param proc_handle A handle for the process to start.
///
/// @return ERR_CODE::NOT_FOUND if the handle does not correlate correctly to a process. ERR_CODE::NO_ERROR otherwise -
///         the process was started successfully.
ERR_CODE syscall_start_process(GEN_HANDLE proc_handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  IRefCounted *om_obj = nullptr;
  task_process *proc_obj = nullptr;

  om_obj = om_retrieve_object(proc_handle);

  if (om_obj == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Process not found\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    proc_obj = dynamic_cast<task_process *>(om_obj);
    if (proc_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Starting ", thread_obj, "\n");
      task_start_process(proc_obj);
      result = ERR_CODE::NO_ERROR;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Cause a process to stop running.
///
/// Has no effect if the process is already stopped. If any of the process's threads are executing on any CPU when this
/// function is called they will be permitted to complete their timeslices before stopping.
///
/// @param proc_handle A handle for the process to stop.
///
/// @return ERR_CODE::NOT_FOUND if the handle does not correlate correctly to a process. ERR_CODE::NO_ERROR otherwise -
///         the process was stopped successfully.
ERR_CODE syscall_stop_process(GEN_HANDLE proc_handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  IRefCounted *om_obj = nullptr;
  task_process *proc_obj = nullptr;

  om_obj = om_retrieve_object(proc_handle);

  if (om_obj == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Thread not found\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    proc_obj = dynamic_cast<task_process *>(om_obj);
    if (proc_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      task_stop_process(proc_obj);
      result = ERR_CODE::NO_ERROR;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Destroy a process
///
/// Destroying a process means it cannot be restarted, and that handles to it become invalid. If the process is running
/// then any running threads will be permitted to complete their timeslices before destruction.
///
/// @param proc_handle A handle to the process to be destroyed.
///
/// @return ERR_CODE::NOT_FOUND if the handle does not refer to a valid process. ERR_CODE::NO_ERROR otherwise - the
///         process has been destroyed successfully.
ERR_CODE syscall_destroy_process(GEN_HANDLE proc_handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  IRefCounted *om_obj = nullptr;
  task_process *proc_obj = nullptr;

  om_obj = om_retrieve_object(proc_handle);

  if (om_obj == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Thread not found\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    proc_obj = dynamic_cast<task_process *>(om_obj);
    if (proc_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      // This also releases the handle's reference to the thread.
      om_remove_object(proc_handle);
      task_destroy_process(proc_obj);
      result = ERR_CODE::NO_ERROR;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Exit the current process immediately.
///
/// All threads within the process will be destroyed and process will end immediately. This is not recommended - if any
/// threads are holding locks they will not be released. The recommended exit strategy is to exit all threads, which
/// will cause the process to exit automatically.
void syscall_exit_process()
{
  KL_TRC_ENTRY;

  task_process *this_proc;
  task_thread *this_thread;
  this_thread = task_get_cur_thread();
  this_proc = this_thread->parent_process;

  task_destroy_process(this_proc);

  panic("Reached end of syscall_exit_process!");
  KL_TRC_EXIT;
}

/// @brief Create a new thread
///
/// Creates a new thread, but the thread does not start running until deliberately started.
///
/// @param[in] entry_point Pointer to a function forming the starting point for the thread.
///
/// @param[out] thread_handle A handle to be used when referring to this thread through the system call interface.
///
/// @return ERR_CODE::INVALID_PARAM if any parameter is invalid. ERR_CODE::INVALID_OP if the thread could not be
///         created for any other reason. ERR_CODE::NO_ERROR if the thread was created successfully.
ERR_CODE syscall_create_thread(void (*entry_point)(), GEN_HANDLE *thread_handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  task_thread *new_thread = nullptr;
  task_thread *cur_thread = nullptr;

  if ((entry_point == nullptr) ||
      (!SYSCALL_IS_UM_ADDRESS(entry_point)) ||
      (thread_handle == nullptr) ||
      (!SYSCALL_IS_UM_ADDRESS(thread_handle)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parameters\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    cur_thread = task_get_cur_thread();
    if ((cur_thread != nullptr) &&
        (cur_thread->parent_process != nullptr) &&
        (cur_thread->parent_process->kernel_mode == false))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Creating thread with entry point ", entry_point, "\n");
      new_thread = task_create_new_thread(entry_point, cur_thread->parent_process);
    }

    if (new_thread != nullptr)
    {
      *thread_handle = om_store_object(new_thread);
      KL_TRC_TRACE(TRC_LVL::FLOW, "New thread (", new_thread, ") created, handle: ", *thread_handle, "\n");

      // The creation of the thread has caused this code to be a reference holder of the thread object, but we want the
      // only reference holder to be the handle - so release our reference now.
      new_thread->ref_release();

      result = ERR_CODE::NO_ERROR;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to create thread\n");
      result = ERR_CODE::INVALID_OP;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Cause a thread to start running.
///
/// Has no effect if the thread is already running.
///
/// @param thread_handle A handle for the thread to start.
///
/// @return ERR_CODE::NOT_FOUND if the handle does not correlate correctly to a thread. ERR_CODE::NO_ERROR otherwise -
///         the thread was started successfully.
ERR_CODE syscall_start_thread(GEN_HANDLE thread_handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  IRefCounted *om_obj = nullptr;
  task_thread *thread_obj = nullptr;

  om_obj = om_retrieve_object(thread_handle);

  if (om_obj == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Thread not found\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    thread_obj = dynamic_cast<task_thread *>(om_obj);
    if (thread_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else if (thread_obj->thread_destroyed)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Can't start destroyed thread\n");
      result = ERR_CODE::INVALID_OP;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Starting ", thread_obj, "\n");
      task_start_thread(thread_obj);
      result = ERR_CODE::NO_ERROR;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Cause a thread to stop running.
///
/// Has no effect if the thread is already stopped. If the thread is executing on any CPU when this function is called
/// it will be permitted to complete its timeslice.
///
/// @param thread_handle A handle for the thread to stop.
///
/// @return ERR_CODE::NOT_FOUND if the handle does not correlate correctly to a thread. ERR_CODE::NO_ERROR otherwise -
///         the thread was stopped successfully.
ERR_CODE syscall_stop_thread(GEN_HANDLE thread_handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  IRefCounted *om_obj = nullptr;
  task_thread *thread_obj = nullptr;

  om_obj = om_retrieve_object(thread_handle);

  if (om_obj == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Thread not found\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    thread_obj = dynamic_cast<task_thread *>(om_obj);
    if (thread_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else if (thread_obj->thread_destroyed)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Can't stop destroyed thread\n");
      result = ERR_CODE::INVALID_OP;
    }
    else
    {
      task_stop_thread(thread_obj);
      result = ERR_CODE::NO_ERROR;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Destroy a thread
///
/// Destroying a thread means it cannot be restarted, and that handles to it become invalid. If the thread is running
/// it will be permitted to complete its timeslice before destruction.
///
/// @param thread_handle A handle to the thread to be destroyed.
///
/// @return ERR_CODE::NOT_FOUND if the handle does not refer to a valid thread. ERR_CODE::NO_ERROR otherwise - the
///         thread has been destroyed successfully.
ERR_CODE syscall_destroy_thread(GEN_HANDLE thread_handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  IRefCounted *om_obj = nullptr;
  task_thread *thread_obj = nullptr;

  om_obj = om_retrieve_object(thread_handle);

  if (om_obj == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Thread not found\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    thread_obj = dynamic_cast<task_thread *>(om_obj);
    if (thread_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      // This also releases the handle's reference to the thread.
      om_remove_object(thread_handle);
      task_destroy_thread(thread_obj);
      result = ERR_CODE::NO_ERROR;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Exit the currently running thread.
///
/// The thread will exit and any threads waiting on it will be signalled.
void syscall_exit_thread()
{
  KL_TRC_ENTRY;

  task_thread *this_thread;
  this_thread = task_get_cur_thread();
  task_destroy_thread(this_thread);

  panic("Reached end of syscall_exit_thread!");
  KL_TRC_EXIT;
}
