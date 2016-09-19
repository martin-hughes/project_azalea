/// @file
/// @brief The kernel's task manager.
///
/// In the OS, the basic unit of execution is a thread. Multiple threads are grouped in to a process. A process defines
/// the address space and permissions of all threads that are associated with it.
///
/// The task manager is responsible for managing the creation and destruction of threads, as well as for scheduling
/// them onto the processor. This is done in a very crude, round robin kind of way.
///
/// The threads (as a task_thread object) point at each other via task_thread::next_thread, in a cycle. The processors
/// move around the cycle until they find a thread that is permitted to run (i.e. not suspended) and not locked (by
/// task_thread::cycle_lock). Being locked means that another processor is about to execute it.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor.h"
#include "processor-int.h"
#include "mem/mem.h"
#include "object_mgr/object_mgr.h"

// This is a list of all processes known to the system.
klib_list process_list;

// The threads each processor is currently running. After initialisation, this points to an array of size equal to the
// number of processors.
task_thread **current_threads = nullptr;

// Should the processor continue running this thread without considering other threads? After initialisation, this
// points to an array of bools equal in size to the number of processors.
static bool *continue_this_thread = nullptr;

// Idle threads for each processor. These are created during initialisation, and after initialisation this is an array
// of pointers equal in size to the number of processors.
task_thread **idle_threads = nullptr;

// A pointer to an arbitrary thread within the cycle of threads. It doesn't really matter which thread this points to,
// the CPUs can just cycle through the cycle to find the one they want.
task_thread *start_of_thread_cycle = nullptr;

// Protects the thread cycle from two threads making simultaneous changes.
kernel_spinlock thread_cycle_lock;

// Some helper functions
void task_thread_cycle_add(task_thread *new_thread);
void task_thread_cycle_remove(task_thread *thread);
void task_thread_cycle_lock();
void task_thread_cycle_unlock();
void task_idle_thread_cycle();

/// @brief Initialise and start the task management subsystem
///
/// Performs setup tasks, and then attaches the task switcher to the periodic timer interrupt, which causes the kernel
/// to start switching processes with no further input.
///
/// The function can return, but task switching should start within a few microseconds. It is the caller's
/// responsibility to monitor this.
///
/// @param kern_start_proc A pointer to a simple function that will be the first process started by the scheduler.
void task_gen_init(ENTRY_PROC kern_start_proc)
{
  KL_TRC_ENTRY;

  mem_process_info *task0_mem_info;
  task_process *first_process;
  unsigned int number_of_procs = proc_mp_proc_count();
  task_thread *new_idle_thread;

  klib_synch_spinlock_init(thread_cycle_lock);

  klib_list_initialize(&process_list);

  task0_mem_info = mem_task_get_task0_entry();
  ASSERT(task0_mem_info != nullptr);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Preparing the processor\n");
  task_platform_init();

  KL_TRC_TRACE(TRC_LVL::FLOW, "Creating first process\n");
  first_process = task_create_new_process(kern_start_proc, true, task0_mem_info);
  KL_TRC_DATA("First process at", (unsigned long)first_process);
  ASSERT(first_process != nullptr);
  task_start_process(first_process);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Creating per-process info\n");
  KL_TRC_DATA("Number of processors", number_of_procs);
  current_threads = new task_thread *[number_of_procs];
  continue_this_thread = new bool[number_of_procs];
  idle_threads = new task_thread *[number_of_procs];
  for (int i = 0; i < number_of_procs; i++)
  {
    KL_TRC_DATA("Working on processor", i);
    current_threads[i] = nullptr;
    continue_this_thread[i] = false;

    new_idle_thread = task_create_new_thread(task_idle_thread_cycle, first_process);
    idle_threads[i] = new_idle_thread;
    task_thread_cycle_remove(idle_threads[i]);
    idle_threads[i]->permit_running = false;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Beginning task switching\n");
  task_install_task_switcher();

  KL_TRC_EXIT;
}

/// @brief Create a new process
///
/// Creates a new process, with an associated thread starting at entry_point. The process remains suspended until
/// deliberately started.
///
/// @param entry_point The entry point for the process's main thread.
///
/// @param kernel_mode TRUE if the process should run entirely within the kernel, FALSE if it's a user mode process.
///
/// @param mem_info If known, this field provides pre-populated memory manager information about this process. For all
///                 processes except the initial kernel start procedure, this should be NULL.
task_process *task_create_new_process(ENTRY_PROC entry_point,
                                      bool kernel_mode,
                                      mem_process_info *mem_info)
{
  task_process *new_process;

  KL_TRC_ENTRY;

  new_process = new task_process;
  KL_TRC_DATA("New process CB at", (unsigned long)new_process);
  klib_list_item_initialize(&new_process->process_list_item);
  klib_list_initialize(&new_process->child_threads);
  new_process->process_list_item.item = (void *)new_process;
  new_process->kernel_mode = kernel_mode;
  new_process->process_id = om_store_object(new_process);
  if (mem_info != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "mem_info provided\n");
    new_process->mem_info = mem_info;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No mem_info, create it\n");
    new_process->mem_info = mem_task_create_task_entry();
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Checks complete, adding process to list\n");
  klib_list_add_head(&process_list, &new_process->process_list_item);

  task_create_new_thread(entry_point, new_process);

  KL_TRC_EXIT;
  return new_process;
}

/// @brief Create a new thread.
///
/// Creates a new thread as part of parent_process, starting at entry_point. The thread remains suspended until it is
/// deliberately started.
///
/// @param entry_point The point that the thread will begin executing from.
///
/// @param parent_process The process this thread is part of.
///
/// @return The new thread
task_thread *task_create_new_thread(ENTRY_PROC entry_point, task_process *parent_process)
{
  KL_TRC_ENTRY;

  task_thread *new_thread = new task_thread;

  ASSERT(parent_process != nullptr);

  KL_TRC_DATA("Entry point", reinterpret_cast<unsigned long>(entry_point));
  KL_TRC_DATA("Parent Process", reinterpret_cast<unsigned long>(parent_process));

  new_thread->parent_process = parent_process;

  klib_list_item_initialize(&new_thread->process_list_item);
  new_thread->process_list_item.item = (void *)new_thread;
  klib_list_add_tail(&parent_process->child_threads, &new_thread->process_list_item);
  new_thread->execution_context = task_int_create_exec_context(entry_point, new_thread);
  new_thread->permit_running = false;
  new_thread->thread_id = om_store_object(new_thread);

  task_thread_cycle_add(new_thread);

  KL_TRC_EXIT;
  return new_thread;
}

/// @brief Destroy a thread
///
/// Stop the thread from executing and destroy it.
///
/// @param unlucky_thread The unlucky thread that is about to be destroyed.
void task_destroy_thread(task_thread *unlucky_thread)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE("Can't destroy threads.");
  KL_TRC_EXIT;
}

/// @brief Destroy a process
///
/// Destroy the process. If it has any threads left within it, they will be destroyed too.
///
/// @param unlucky_process The process to destroy.
void task_destroy_process(task_process *unlucky_process)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE("Can't destroy processes.");
  KL_TRC_EXIT;
}

/// @brief The main task scheduler
///
/// This code is called from the processor-specific part of the code whenever it wants to schedule another thread. This
/// function selects the next thread that should execute it and passes that back to the caller. The caller is then
/// responsible for actually scheduling it - **NOTE** This code assumes that the thread is scheduled, the caller must
/// not simply abandon it.
///
/// The scheduling algorithm is very simply, and pays no attention to demand, CPU load, caching niceties or anything
/// else. The threads are linked in a cycle via task_thread::next_thread, and the CPUs move through this cycle. If they
/// are executing the thread, they gain a lock on task_thread::cycle_lock to indicate this. A CPU looking through the
/// cycle will skip over all threads currently locked or which are suspended (via task_thread::permit_running)
///
/// If a CPU cannot find a valid thread, it will execute an idle thread (stored in idle_threads) which effectively put
/// the processor to sleep via a HLT-loop.
///
/// @return The thread that the caller **MUST** begin executing.
task_thread *task_get_next_thread()
{
  task_thread *next_thread = nullptr;
  task_thread *start_thread = nullptr;
  unsigned int proc_id;
  bool found_thread;
  KL_TRC_ENTRY;

  proc_id = proc_mp_this_proc_id();

  ASSERT(continue_this_thread != nullptr);
  ASSERT(current_threads != nullptr);

  if (continue_this_thread[proc_id])
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Requested to continue current thread\n");
    next_thread = current_threads[proc_id];
    ASSERT(next_thread != nullptr);
  }
  else
  {
    if (current_threads[proc_id] == idle_threads[proc_id])
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Currently on an idle thread, attempt to find non-idle thread\n");
      ASSERT(start_of_thread_cycle != nullptr);
      next_thread = start_of_thread_cycle;
    }
    else if (current_threads[proc_id] == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No threads running previously, start at the beginning\n");
      ASSERT(start_of_thread_cycle != nullptr);
      next_thread = start_of_thread_cycle;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Try next thread.\n");
      next_thread = current_threads[proc_id]->next_thread;
      ASSERT(next_thread != nullptr);
    }

    start_thread = next_thread;
    ASSERT(start_thread != nullptr);
    found_thread = false;
    do
    {
      KL_TRC_DATA("Considering thread", (unsigned long)next_thread);
      if ((next_thread->permit_running) && (next_thread->cycle_lock != 1))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Trying to lock for ourselves... ");
        if (klib_synch_spinlock_try_lock(next_thread->cycle_lock))
        {
          // Having locked it, double check that it's still OK to run, otherwise release it and carry on
          if (next_thread->permit_running)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "SUCCESS!\n");
            found_thread = true;
            break;
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Had to release it again\n");
            klib_synch_spinlock_unlock(next_thread->cycle_lock);
          }
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Not this time, continue\n");
        }
      }

      next_thread = next_thread->next_thread;
      ASSERT(next_thread != nullptr)
    } while (next_thread != start_thread);

    if (found_thread)
    {
      KL_TRC_DATA("The next thread to execute is live thread", (unsigned long)next_thread);
      if ((next_thread != current_threads[proc_id]) && (current_threads[proc_id] != nullptr))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Unlocking old thread\n");
        klib_synch_spinlock_unlock(current_threads[proc_id]->cycle_lock);
      }
    }
    else
    {
      if ((current_threads[proc_id] != nullptr) && (current_threads[proc_id]->permit_running))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Stick with our current thread\n");
        next_thread = current_threads[proc_id];
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "No thread found, switch to idle thread\n");
        if (current_threads[proc_id] != nullptr)
        {
          klib_synch_spinlock_unlock(current_threads[proc_id]->cycle_lock);
        }
        next_thread = idle_threads[proc_id];
      }
    }
  }

  current_threads[proc_id] = next_thread;

  KL_TRC_DATA("Next thread (addr)", (unsigned long)next_thread);
  KL_TRC_EXIT;

  return next_thread;
}

/// @brief Return pointer to the currently executing thread
///
/// @return The task_thread object of the executing thread on this processor, or NULL if we haven't got that far yet.
task_thread *task_get_cur_thread()
{
  KL_TRC_ENTRY;

  unsigned int proc_id;

  task_thread *ret_thread;
  if (current_threads == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No threads running yet, return NULL\n");
    ret_thread = nullptr;
  }
  else
  {
    proc_id = proc_mp_this_proc_id();
    KL_TRC_DATA("Getting thread for processor ID", proc_id);
    ret_thread = current_threads[proc_id];
    KL_TRC_DATA("Returning thread", (unsigned long)ret_thread);
  }

  KL_TRC_EXIT;

  return ret_thread;
}

/// @brief Return the thread ID of the currently executing thread
///
/// @return The thread ID of the currently executing thread, or 0 if we haven't begun tasking yet.
THREAD_ID task_current_thread_id()
{
  KL_TRC_ENTRY;

  task_thread *thread = task_get_cur_thread();
  THREAD_ID ti;

  if (thread == nullptr)
  {
    ti = 0;
  }
  else
  {
    ti = thread->thread_id;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Returning thread ID: ", ti, "\n");

  KL_TRC_EXIT;
  return ti;
}

/// @brief Return the process ID of the currently executing thread
///
/// @return The process ID of the currently executing thread, or 0 if we haven't begun tasking yet.
PROCESS_ID task_current_process_id()
{
  KL_TRC_ENTRY;

  task_thread *thread = task_get_cur_thread();
  PROCESS_ID pi;

  if (thread == nullptr)
  {
    pi = 0;
  }
  else
  {
    ASSERT(thread->parent_process != nullptr);
    pi = thread->parent_process->process_id;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Returning process ID: ", pi, "\n");

  KL_TRC_EXIT;
  return pi;
}

/// @brief Get the task_process block matching a given process ID
///
/// @param proc_id The process ID to return the data block for
///
/// @return The task_process data block
task_process *task_get_process_data(PROCESS_ID proc_id)
{
  task_process *ret_proc;
  KL_TRC_ENTRY;
  ret_proc = (task_process *)om_retrieve_object(proc_id);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Process ID ", proc_id, " correlates to data block ", ret_proc, "\n");
  KL_TRC_EXIT;
  return ret_proc;
}

/// @brief Get the task_thread block matching a given thread ID
///
/// @param thread_id The thread ID to return the data block for
///
/// @return The task_thread data block
task_thread *task_get_thread_data(THREAD_ID thread_id)
{
  task_process *ret_thread;
  KL_TRC_ENTRY;
  ret_thread = (task_thread *)om_retrieve_object(thread_id);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Thread ID ", thread_id, " correlates to data block ", ret_thread, "\n");
  KL_TRC_EXIT;
  return ret_thread;
}

/// @brief Start executing all threads within the given process
///
/// @param process The process whose threads will execute.
void task_start_process(task_process *process)
{
  KL_TRC_ENTRY;

  task_thread *next_thread = nullptr;
  klib_list_item *next_item = nullptr;

  KL_TRC_DATA("Process address", (unsigned long)process);

  ASSERT(process != nullptr);
  next_item = process->child_threads.head;

  while (next_item != nullptr)
  {
    next_thread = (task_thread *)next_item->item;
    ASSERT(next_thread != nullptr);
    KL_TRC_DATA("Next thread", (unsigned long)next_thread);
    task_start_thread(next_thread);

    next_item = next_item->next;
  }

  KL_TRC_EXIT;
}

/// @brief Stop all threads in a process
///
/// @param process The process whose threads will be stopped.
void task_stop_process(task_process *process)
{
  KL_TRC_ENTRY;

  task_thread *next_thread = nullptr;
  klib_list_item *next_item = nullptr;

  KL_TRC_DATA("Process address", (unsigned long)process);

  ASSERT(process != nullptr);
  next_item = process->child_threads.head;

  while (next_item != nullptr)
  {
    next_thread = (task_thread *)next_item->item;
    ASSERT(next_thread != nullptr);
    KL_TRC_DATA("Next thread", (unsigned long)next_thread);
    task_stop_thread(next_thread);

    next_item = next_item->next;
  }

  KL_TRC_EXIT;
}

/// @brief All a given thread to execute
///
/// Flag the given thread as being runnable, so that the scheduler will schedule it. It may not start immediately, as
/// the scheduler will execute threads in order, but it will execute at some point in the future.
///
/// @param thread The thread to start
void task_start_thread(task_thread *thread)
{
  KL_TRC_ENTRY;
  KL_TRC_DATA("Thread address", (unsigned long)thread);

  ASSERT(thread != nullptr);
  if(!thread->permit_running)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Thread not running, start it\n");
    thread->permit_running = true;
  }

  KL_TRC_EXIT;
}

/// @brief Stop a specific thread
///
/// Stop the specified thread from executing. It will continue until the end of this timeslice if it is currently
/// running on any CPU.
///
/// @param thread The thread to stop.
void task_stop_thread(task_thread *thread)
{
  KL_TRC_ENTRY;
  KL_TRC_DATA("Thread address", (unsigned long)thread);

  ASSERT(thread != nullptr);
  if(thread->permit_running)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Thread running, stop it\n");
    thread->permit_running = false;
  }

  KL_TRC_EXIT;
}

/// @brief Lock this thread to this CPU for now
///
/// Force the scheduler to continually re-schedule **this** thread, rather than selecting a new one at the end of its
/// timeslice. This is only really intended to be used by the kernel's synchronisation code, to ensure that it can't be
/// preempted in a state where it would be left in a deadlock.
void task_continue_this_thread()
{
  KL_TRC_ENTRY;

  continue_this_thread[proc_mp_this_proc_id()] = true;

  KL_TRC_EXIT;
}

/// @brief Cancels the effect of task_continue_this_thread()
void task_resume_scheduling()
{
  KL_TRC_ENTRY;

  continue_this_thread[proc_mp_this_proc_id()] = false;

  KL_TRC_EXIT;
}

/// @brief Give up the rest of our time slice.
void task_yield()
{
  KL_TRC_ENTRY;

  asm("hlt");

  KL_TRC_EXIT;
}

/// @brief Add a new thread to the cycle of all threads
///
/// All threads are joined in a cycle by task_thread::next_thread. Add new_thread to this cycle.
///
/// @param new_thread The new thread to add.
void task_thread_cycle_add(task_thread *new_thread)
{
  KL_TRC_ENTRY;
  // We don't need to lock for the scheduler - the cycle is always in a consistent state as far as it's concerned, but
  // we do need to prevent two threads being added at once, since that might cause one or the other to be lost.
  task_thread_cycle_lock();

  if (start_of_thread_cycle == nullptr)
  {
    start_of_thread_cycle = new_thread;
    start_of_thread_cycle->next_thread = new_thread;
  }
  else
  {
    new_thread->next_thread = start_of_thread_cycle->next_thread;
    start_of_thread_cycle->next_thread = new_thread;
  }

  task_thread_cycle_unlock();
  KL_TRC_EXIT;
}

/// @brief Remove a thread from the thread cycle
///
/// This will cause it to not be considered for execution any more.
///
/// @param thread The thread to remove.
void task_thread_cycle_remove(task_thread *thread)
{
  KL_TRC_ENTRY;

  task_thread *search_thread;

  task_thread_cycle_lock();

  // Special case here - we're deleting the last thread.
  if (thread->next_thread == thread)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Deleting the last thread\n");
    start_of_thread_cycle = nullptr;
  }
  else
  {
    if (start_of_thread_cycle == thread)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Moving start of thread cycle out of the way of the dead thread\n");
      start_of_thread_cycle = thread->next_thread;
    }

    search_thread = start_of_thread_cycle;
    while (search_thread->next_thread != thread)
    {
      search_thread = search_thread->next_thread;

      if (search_thread == start_of_thread_cycle)
      {
        // If this isn't true then the thread officially didn't exist to start with.
        ASSERT(search_thread->next_thread == thread);
      }
    }

    search_thread->next_thread = search_thread->next_thread->next_thread;
  }

  task_thread_cycle_unlock();
  KL_TRC_EXIT;
}

/// @brief Lock the thread cycle
///
/// This is used when editing the thread cycle, to ensure constant consistency.
void task_thread_cycle_lock()
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(thread_cycle_lock);

  KL_TRC_EXIT;
}

/// @brief Unlock the thread cycle
void task_thread_cycle_unlock()
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_unlock(thread_cycle_lock);

  KL_TRC_EXIT;
}

/// @brief The idle thread's code
///
/// This function is executed by every one of the idle threads belonging to each processor.
void task_idle_thread_cycle()
{
  while(1)
  {
    asm("hlt");
  }
}
