/// @file
/// @brief Task management code specific to process objects.

//#define ENABLE_TRACING

#include "processor.h"
#include "processor-int.h"
#include "../system_tree/fs/proc/proc_fs.h"
#include "system_tree.h"

#include <stdio.h>

/// @brief Create a new process
///
/// Creates a new process, with an associated thread starting at entry_point. The process remains suspended until
/// deliberately started. This should not be called directly - use the static create() function.
///
/// @param entry_point The entry point for the process's main thread.
///
/// @param kernel_mode TRUE if the process should run entirely within the kernel, FALSE if it's a user mode process.
///
/// @param mem_info If known, this field provides pre-populated memory manager information about this process. For all
///                 processes except the initial kernel start procedure, this should be NULL.
task_process::task_process(ENTRY_PROC entry_point, bool kernel_mode, mem_process_info *mem_info) :
  kernel_mode{kernel_mode},
  being_destroyed{false},
  has_ever_started{false}
{
  KL_TRC_ENTRY;

  klib_list_initialize(&this->child_threads);

  if (mem_info != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "mem_info provided\n");
    this->mem_info = mem_info;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No mem_info, create it\n");
    this->mem_info = mem_task_create_task_entry();
    mem_vmm_allocate_specific_range(0, 1, this);
  }

  KL_TRC_EXIT;
}

/// @brief Create a new process.
///
/// @param entry_point Pointer to the first instruction that should be executed in this process.
///
/// @param kernel_mode Should this be a kernel-mode process?
///
/// @param mem_info If there is a pre-defined mem_process_info for this process then it should be provided here,
///                 otherwise use nullptr.
///
/// @return A shared_ptr to the new process.
std::shared_ptr<task_process> task_process::create(ENTRY_PROC entry_point,
                                                   bool kernel_mode,
                                                   mem_process_info *mem_info)
{
  std::shared_ptr<task_thread> first_thread;
  std::shared_ptr<IHandledObject> leaf_ptr;
  char proc_path_ptr_buffer[34];

  KL_TRC_ENTRY;

  // Construct the process object.
  std::shared_ptr<task_process> new_proc = std::shared_ptr<task_process>(
    new task_process(entry_point, kernel_mode, mem_info));

  // Add it to the "proc" tree of processes.
  std::shared_ptr<IHandledObject> branch_ptr;
  std::shared_ptr<proc_fs_root_branch> proc_fs_root_ptr;
  system_tree()->get_child("\\proc", branch_ptr);
  proc_fs_root_ptr = std::dynamic_pointer_cast<proc_fs_root_branch>(branch_ptr);
  ASSERT(proc_fs_root_ptr);
  proc_fs_root_ptr->add_process(new_proc);

  // Create a thread associated with it.
  KL_TRC_TRACE(TRC_LVL::FLOW, "Create new thread\n");
  first_thread = task_thread::create(entry_point, new_proc);
  ASSERT(first_thread != nullptr);

  if (task_get_cur_thread() != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not in initial startup, look for stdio pipes\n");

    // If the current process has stdout, stdin or stderr pipes, use those for the newly created process too.
    if (system_tree()->get_child("\\proc\\0\\stdout", leaf_ptr) == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Copy stdout from parent to child\n");
      snprintf(proc_path_ptr_buffer, sizeof(proc_path_ptr_buffer), "\\proc\\%p\\stdout", new_proc.get());
      system_tree()->add_child(proc_path_ptr_buffer, leaf_ptr);
    }

    if (system_tree()->get_child("\\proc\\0\\stdin", leaf_ptr) == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Copy stdin from parent to child\n");
      snprintf(proc_path_ptr_buffer, sizeof(proc_path_ptr_buffer), "\\proc\\%p\\stdin", new_proc.get());
      system_tree()->add_child(proc_path_ptr_buffer, leaf_ptr);
    }

    if (system_tree()->get_child("\\proc\\0\\stderr", leaf_ptr) == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Copy stderr from parent to child\n");
      snprintf(proc_path_ptr_buffer, sizeof(proc_path_ptr_buffer), "\\proc\\%p\\stderr", new_proc.get());
      system_tree()->add_child(proc_path_ptr_buffer, leaf_ptr);
    }
  }

  KL_TRC_EXIT;
  return new_proc;
}

task_process::~task_process()
{
  // Make sure the process was destroyed via destroy_process.
  ASSERT(this->being_destroyed);

  // Free all memory associated with this process. This is safe because this destructor is never run in the context of
  // the process being destroyed - it either runs as part of proc_tidyup_thread or that of the thread that started the
  // destruction of the process.
  mem_task_free_task(this);
}

/// @brief Final destruction of a process.
///
/// Destroys all threads and then signals anyone waiting for this process to finish.
///
/// @param exit_code The exit code to assign to this process.
void task_process::destroy_process(uint64_t exit_code)
{
  KL_TRC_ENTRY;

  klib_list_item <std::shared_ptr<task_thread>> *list_item;
  klib_list_item <std::shared_ptr<task_thread>> *next_item;
  bool skipped_this_thread = false;

  if (!this->being_destroyed)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Destroying process\n");
    this->being_destroyed = true;

    this->exit_code = exit_code;

    // This allows other parts of the system to set a failed status, if needed.
    if (this->proc_status == OPER_STATUS::OK)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set status to stopped\n");
      this->proc_status = OPER_STATUS::STOPPED;
    }

    this->signal_event();

    std::shared_ptr<IHandledObject> branch_ptr;
    std::shared_ptr<proc_fs_root_branch> proc_fs_root_ptr;
    system_tree()->get_child("\\proc", branch_ptr);
    proc_fs_root_ptr = std::dynamic_pointer_cast<proc_fs_root_branch>(branch_ptr);
    ASSERT(proc_fs_root_ptr);
    proc_fs_root_ptr->remove_process(shared_from_this());

    // Destroy all threads except for this one.
    // this needs more locking! But that can wait for the re-write.
    list_item = this->child_threads.head;
    while (list_item != nullptr)
    {
      ASSERT(list_item->list_obj != nullptr);
      next_item = list_item->next;
      if (list_item->item.get() != task_get_cur_thread())
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Destroying thread: ", list_item->list_obj, "\n");
        list_item->item->destroy_thread();
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Destroy this thread later\n");
        skipped_this_thread = true;
      }

      list_item = next_item;
    }


    if (skipped_this_thread)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Destroying this thread now\n");
      task_get_cur_thread()->destroy_thread();
    }

  }

  KL_TRC_EXIT;
}


/// @brief Start executing all threads within the given process
///
void task_process::start_process()
{
  KL_TRC_ENTRY;

  has_ever_started = true;

  std::shared_ptr<task_thread> next_thread = nullptr;
  klib_list_item<std::shared_ptr<task_thread>> *next_item = nullptr;

  next_item = this->child_threads.head;

  while (next_item != nullptr)
  {
    next_thread = next_item->item;
    ASSERT(next_thread != nullptr);
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Next thread", next_thread.get(), "\n");
    next_thread->start_thread();

    next_item = next_item->next;
  }

  KL_TRC_EXIT;
}

/// @brief Stop all threads in a process
///
void task_process::stop_process()
{
  KL_TRC_ENTRY;

  std::shared_ptr<task_thread>next_thread = nullptr;
  klib_list_item<std::shared_ptr<task_thread>> *next_item = nullptr;

  next_item = this->child_threads.head;

  while (next_item != nullptr)
  {
    next_thread = next_item->item;
    ASSERT(next_thread != nullptr);
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Next thread", (uint64_t)next_thread.get(), "\n");
    next_thread->stop_thread();

    next_item = next_item->next;
  }

  KL_TRC_EXIT;
}

/// @brief Called by task_thread when it is created, to add itself to the process's thread list.
///
/// @param new_thread The newly created thread.
void task_process::add_new_thread(std::shared_ptr<task_thread> new_thread)
{
  KL_TRC_ENTRY;

  klib_list_add_tail(&this->child_threads, new_thread->process_list_item);

  KL_TRC_EXIT;
}

/// @brief Called by task_thread when it is ending to allow the process to be aware of its demise.
///
/// If it is the last remaining thread, then the process will destroy itself.
///
/// @param thread The thread that is ending.
void task_process::thread_ending(task_thread *thread)
{
  KL_TRC_ENTRY;

  ASSERT(thread != nullptr);
  klib_list_remove(thread->process_list_item);

  if (klib_list_get_length(&this->child_threads) == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No more threads\n");
    this->destroy_process(0); // In this case, we haven't been provided with an exit code, so just assume.
  }

  KL_TRC_EXIT;
}

/// @brief Stores messages for retrieval by a user-mode process.
///
/// This is unlike other objects where messages are handled directly by this handler - we don't have a facility to call
/// directly back to user mode code yet. As such, we just take the message from the global queue and add it to our
/// internal queue.
///
/// Only "basic" messages can be sent to processes at the moment.
///
/// @param message The message to be handled by the process.
void task_process::handle_message(std::unique_ptr<msg::root_msg> &message)
{
  msg::basic_msg *msg_ptr;

  KL_TRC_ENTRY;

  msg_ptr = dynamic_cast<msg::basic_msg *>(message.get());
  if (msg_ptr && this->messaging.accepts_msgs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found a basic message, queue it for later handling\n");
    message.release();
    std::unique_ptr<msg::basic_msg> msg_u_ptr(msg_ptr);

    ipc_raw_spinlock_lock(this->messaging.message_lock);
    this->messaging.message_queue.push(std::move(msg_u_ptr));
    ipc_raw_spinlock_unlock(this->messaging.message_lock);
  }

  KL_TRC_EXIT;
}

void task_process::add_to_dead_list()
{
  task_process *old_head;
  KL_TRC_ENTRY;

  this->in_dead_list = true;

  old_head = dead_processes;

  do
  {
    this->next_defunct_process = old_head;
  } while (!std::atomic_compare_exchange_weak(&dead_processes, &old_head, this));

  KL_TRC_EXIT;
}
