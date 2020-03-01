/// @file
/// @brief Process and thread control part of the system call interface

//#define ENABLE_TRACING

#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include "object_mgr/object_mgr.h"
#include "klib/klib.h"

/// @brief Create a new process
///
/// Creates a new process. The new process contains one thread which will start at `entry_point_addr`. No memory is
/// mapped apart from a single stack for the initial thread, so it will be necessary to map and populate memory pages
/// before calling `syscall_start_process`.
///
/// @param[in] entry_point_addr The virtual memory starting address for the new process, in the context of the new
///                             process. As noted above, it is necessary to create the memory mappings to fulfil this
///                             entry point before calling `syscall_start_process`
///
/// @param[out] proc_handle Storage for a handle to the new process.
///
/// @return ERR_CODE::INVALID_PARAM if either parameter contains an invalid address. ERR_CODE::NO_ERROR otherwise.
ERR_CODE syscall_create_process(void *entry_point_addr, GEN_HANDLE *proc_handle)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  std::shared_ptr<task_process> new_process;
  task_thread *cur_thread = task_get_cur_thread();

  KL_TRC_ENTRY;

  if ((entry_point_addr == nullptr) ||
      (!SYSCALL_IS_UM_ADDRESS(entry_point_addr)) ||
      (proc_handle == nullptr) ||
      (!SYSCALL_IS_UM_ADDRESS(proc_handle)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parameters\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    object_data new_object;
    new_process = task_process::create(reinterpret_cast<ENTRY_PROC>(entry_point_addr));

    std::shared_ptr<IHandledObject> proc_ptr = std::dynamic_pointer_cast<IHandledObject>(new_process);
    new_object.object_ptr = proc_ptr;
    *proc_handle = cur_thread->thread_handles.store_object(new_object);
    KL_TRC_TRACE(TRC_LVL::FLOW, "New process (", new_process.get(), ") created, handle: ", *proc_handle, "\n");

    result = ERR_CODE::NO_ERROR;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Setup argc, argv and the environment for a newly created process.
///
/// Once a process has been started for the first time, this system call fail.
///
/// @param proc_handle The process to set parameters for.
///
/// @param argc The conventional C argc.
///
/// @param argv_ptr Pointer to traditional argv array.
///
/// @param environ_ptr Pointer to the traditional environment array.
///
/// @return A suitable error code - ERR_CODE::INVALID_OP if the process is already running.
ERR_CODE syscall_set_startup_params(GEN_HANDLE proc_handle, uint64_t argc, uint64_t argv_ptr, uint64_t environ_ptr)
{
  ERR_CODE result = ERR_CODE::NO_ERROR;
  std::shared_ptr<task_process> proc_obj;
  task_thread *cur_thread = task_get_cur_thread();

  KL_TRC_ENTRY;

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    proc_obj = std::dynamic_pointer_cast<task_process>(
      cur_thread->thread_handles.retrieve_handled_object(proc_handle));

    if (proc_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else if (proc_obj->has_ever_started)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Can't set parameters for a task that has already been started before\n");
      result = ERR_CODE::INVALID_OP;
    }
    else
    {
      task_set_start_params(proc_obj.get(),
                            argc,
                            reinterpret_cast<char **>(argv_ptr),
                            reinterpret_cast<char **>(environ_ptr));
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
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
  std::shared_ptr<task_process> proc_obj;
  task_thread *cur_thread = task_get_cur_thread();

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    proc_obj = std::dynamic_pointer_cast<task_process>(
      cur_thread->thread_handles.retrieve_handled_object(proc_handle));

    if (proc_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Starting ", proc_obj.get(), "\n");
      proc_obj->start_process();
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
  std::shared_ptr<task_process> proc_obj;
  task_thread *cur_thread = task_get_cur_thread();


  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    proc_obj = std::dynamic_pointer_cast<task_process>(
      cur_thread->thread_handles.retrieve_handled_object(proc_handle));

    if (proc_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      proc_obj->stop_process();
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
  std::shared_ptr<task_process> proc_obj;
  task_thread *cur_thread = task_get_cur_thread();

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    proc_obj = std::dynamic_pointer_cast<task_process>(
      cur_thread->thread_handles.retrieve_handled_object(proc_handle));

    if (proc_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      cur_thread->thread_handles.remove_object(proc_handle);
      proc_obj->destroy_process();
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

  // Get a pointer to the process object instead of a shared one, because exiting the process means the shared pointer
  // object will never go out of scope, leaving the process object hanging forever. Using the raw pointer is safe
  // because we know the process must exist at the moment, since we're running in it.
  this_thread = task_get_cur_thread();
  this_proc = this_thread->parent_process.get();

  this_proc->destroy_process();

  // This panic should never hit because destroy_process() should kill this process and never return.
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
  std::shared_ptr<task_thread> new_thread = nullptr;
  task_thread *cur_thread = task_get_cur_thread();

  if ((entry_point == nullptr) ||
      (!SYSCALL_IS_UM_ADDRESS(entry_point)) ||
      (thread_handle == nullptr) ||
      (!SYSCALL_IS_UM_ADDRESS(thread_handle)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parameters\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    if ((cur_thread->parent_process != nullptr) &&
        (cur_thread->parent_process->kernel_mode == false))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Creating thread with entry point ", entry_point, "\n");
      new_thread = task_thread::create(entry_point, cur_thread->parent_process);
    }

    if (new_thread != nullptr)
    {
      object_data new_object;
      new_object.object_ptr = new_thread;
      *thread_handle = cur_thread->thread_handles.store_object(new_object);
      KL_TRC_TRACE(TRC_LVL::FLOW, "New thread (", new_thread.get(), ") created, handle: ", *thread_handle, "\n");

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
  bool thread_started = false;
  std::shared_ptr<task_thread> thread_obj = nullptr;
  task_thread *cur_thread = task_get_cur_thread();

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    thread_obj = std::dynamic_pointer_cast<task_thread>(
      cur_thread->thread_handles.retrieve_handled_object(thread_handle));

    if (thread_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Starting ", thread_obj.get(), "\n");
      thread_started = thread_obj->start_thread();
      if (thread_started)
      {
        result = ERR_CODE::NO_ERROR;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't start thread - it is being destroyed\n");
        result = ERR_CODE::INVALID_OP;
      }
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
///         the thread was stopped successfully, or was stopped already.
ERR_CODE syscall_stop_thread(GEN_HANDLE thread_handle)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;
  std::shared_ptr<task_thread> thread_obj;
  task_thread *cur_thread = task_get_cur_thread();

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    thread_obj = std::dynamic_pointer_cast<task_thread>(
      cur_thread->thread_handles.retrieve_handled_object(thread_handle));

    if (thread_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      thread_obj->stop_thread();
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
  std::shared_ptr<task_thread> thread_obj;
  task_thread *cur_thread = task_get_cur_thread();

  if (cur_thread == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Couldn't identify current thread\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    thread_obj = std::dynamic_pointer_cast<task_thread>(
      cur_thread->thread_handles.retrieve_handled_object(thread_handle));

    if (thread_obj == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong object type\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      // This also releases the handle's reference to the thread.
      cur_thread->thread_handles.remove_object(thread_handle);
      thread_obj->destroy_thread();
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
  this_thread->destroy_thread();

  panic("Reached end of syscall_exit_thread!");
  KL_TRC_EXIT;
}
