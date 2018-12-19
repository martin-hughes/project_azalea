/// @file
/// @brief Task management code specific to threads.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor.h"
#include "processor-int.h"
#include "object_mgr/object_mgr.h"

/// @brief Create a new thread.
///
/// Creates a new thread as part of `parent`, starting at entry_point. The thread remains suspended until it is
/// deliberately started.
///
/// @param entry_point The point that the thread will begin executing from.
///
/// @param parent The process this thread is part of.
task_thread::task_thread(ENTRY_PROC entry_point, std::shared_ptr<task_process> parent) :
  permit_running(false),
  parent_process(parent),
  thread_destroyed(false)
{
  KL_TRC_ENTRY;
  ASSERT(parent_process != nullptr);

  this->execution_context = task_int_create_exec_context(entry_point, this);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Context created @ ", this->execution_context, "\n");
  this->process_list_item = new klib_list_item<std::shared_ptr<task_thread>>();
  this->synch_list_item = new klib_list_item<std::shared_ptr<task_thread>>();

  if (!parent_process->being_destroyed)
  {
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Entry point: ", reinterpret_cast<uint64_t>(entry_point), "\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Parent Process: ", reinterpret_cast<uint64_t>(parent_process.get()), "\n");

    klib_list_item_initialize(this->process_list_item);
    klib_list_item_initialize(this->synch_list_item);

    klib_synch_spinlock_init(this->cycle_lock);
    task_thread_cycle_add(this);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Don't bother creating thread - process being destroyed\n");
  }

  KL_TRC_EXIT;
}

std::shared_ptr<task_thread> task_thread::create(ENTRY_PROC entry_point, std::shared_ptr<task_process> parent)
{
  KL_TRC_ENTRY;

  std::shared_ptr<task_thread> new_thread = std::shared_ptr<task_thread>(new task_thread(entry_point, parent));

  new_thread->synch_list_item->item = new_thread;
  new_thread->process_list_item->item = new_thread;

  parent->add_new_thread(new_thread);

  KL_TRC_EXIT;

  return new_thread;
}

task_thread::~task_thread()
{
  KL_TRC_ENTRY;
  ASSERT(thread_destroyed);
  task_int_delete_exec_context(this);
  delete this->process_list_item;
  delete this->synch_list_item;
  this->parent_process = nullptr;

  KL_TRC_EXIT;
}

/// @brief Parts of the thread destruction handled by the thread class.
///
/// This code currently only triggers any threads that were waiting for the termination of this one.
void task_thread::destroy_thread()
{
  bool destroying_this_thread;

  KL_TRC_ENTRY;

  if (!this->thread_destroyed)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Destroying thread.\n");
    this->thread_destroyed = true;
    this->trigger_all_threads();

    destroying_this_thread = (task_get_cur_thread() == this);

    if (!destroying_this_thread)
    {
      // Stop the thread from running, then wait for it to be unscheduled.
      KL_TRC_TRACE(TRC_LVL::FLOW, "Destroy another thread...\n");
      this->stop_thread();
      klib_synch_spinlock_lock(this->cycle_lock);
      task_thread_cycle_remove(this);
    }

    this->parent_process->thread_ending(this);
    ASSERT(this->synch_list_item->item != nullptr);
    this->process_list_item->item = nullptr;

    if (klib_list_item_is_in_any_list(this->synch_list_item))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Remove from synch list");
      klib_list_remove(this->synch_list_item);
    }

    if (destroying_this_thread)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Abandoning this thread.");

      task_continue_this_thread();
      task_thread_cycle_remove(this);
      this->stop_thread();
      klib_list_add_tail(&dead_thread_list, this->synch_list_item);
      task_resume_scheduling();
      task_yield();

      panic("Came back from abandoning a thread!");
    }
    else
    {
      this->synch_list_item->item = nullptr;
    }
  }

  KL_TRC_EXIT;
}

/// @brief Give this thread a chance to execute
///
/// Flag this thread as being runnable, so that the scheduler will schedule it. It may not start immediately, as the
/// scheduler will execute threads in order, but it will execute at some point in the future.
///
/// @return True if the thread was flagged to run, or was running already. False if not - the thread is being
///         destroyed.
bool task_thread::start_thread()
{
  bool result = false;
  KL_TRC_ENTRY;

  if (!this->thread_destroyed)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Flag thread to run.\n");
    this->permit_running = true;
    result = true;
  }

  KL_TRC_EXIT;
  return result;
}

/// @brief Stop this thread
///
/// Stop this thread from executing. It will continue until the end of this timeslice if it is currently running on any
/// CPU.
///
/// @return True - to indicate the thread was flagged to be stopped.
bool task_thread::stop_thread()
{
  KL_TRC_ENTRY;

  if(this->permit_running)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Thread running, stop it\n");
    this->permit_running = false;
  }

  KL_TRC_EXIT;

  return true;
}
