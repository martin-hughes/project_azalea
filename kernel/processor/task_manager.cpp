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
///
/// Notice that much of the code in this file is contained within functions, rather than being delegated to the classes
/// of the relevant objects. This is simply because of how this code comes from very early on in the project - it may
/// well change one day.

// Known defects:
// - There's a possible race condition where waiting for a thread just as it is about to be destroyed may cause the
//   waiting thread to have to wait until the object is destroyed until it gets signalled, rather than being
//   signalled at the initial destruction.
// - It is possible to create a thread just as the process is being destroyed.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor.h"
#include "processor-int.h"
#include "mem/mem.h"
#include "object_mgr/object_mgr.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/proc/proc_fs.h"

#ifdef _MSVC_LANG
#include <intrin.h>
#endif

#ifdef _MSVC_LANG
#include <intrin.h>
#endif

namespace
{

  // The threads each processor is currently running. After initialisation, this points to an array of size equal to
  // the number of processors.
  task_thread **current_threads = nullptr;

  // Should the processor continue running this thread without considering other threads? After initialisation, this
  // points to an array of bools equal in size to the number of processors.
  static bool *continue_this_thread = nullptr;

  // Idle threads for each processor. These are created during initialisation, and after initialisation this is an
  // array of pointers equal in size to the number of processors.
  task_thread **idle_threads = nullptr;

  // A pointer to an arbitrary thread within the cycle of threads. It doesn't really matter which thread this points
  // to, the CPUs can just cycle through the cycle to find the one they want.
  task_thread *start_of_thread_cycle = nullptr;

  // Protects the thread cycle from two threads making simultaneous changes.
  kernel_spinlock thread_cycle_lock;
}

// Should the task manager simply abandon this thread when it comes up for rescheduling? This isn't in the file's
// private namespace because the lower-level code may want to examine it.
bool *abandon_thread = nullptr;


/// @brief Initialise and start the task management subsystem
///
/// This function initialises the task manager and creates a system process consisting of idle threads and the IRQ
/// slowpath thread. It does not start the tasking system.
///
/// @return The system process created by this procedure.
std::shared_ptr<task_process> task_init()
{
  KL_TRC_ENTRY;

  std::shared_ptr<task_process> system_process;

  task_gen_init();
  system_process = task_create_system_process();

  ASSERT(system_process != nullptr);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "System process: ", system_process, "\n");
  KL_TRC_EXIT;

  return system_process;
}

/// @brief General initialisation of the task manager system.
///
/// The task manager will not function correctly until the idle threads have been created (using
/// task_create_system_process() ) and another process has been started.
void task_gen_init()
{
  KL_TRC_ENTRY;

  ERR_CODE ec;

  uint32_t number_of_procs = proc_mp_proc_count();

  klib_synch_spinlock_init(thread_cycle_lock);

  std::shared_ptr<proc_fs_root_branch> proc_fs_root_ptr;
  proc_fs_root_ptr = std::make_shared<proc_fs_root_branch>();
  ec = system_tree()->add_branch("proc", proc_fs_root_ptr);
  ASSERT(ec == ERR_CODE::NO_ERROR);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Preparing the processor\n");
  task_platform_init();

  KL_TRC_TRACE(TRC_LVL::FLOW, "Creating per-process info\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Number of processors", number_of_procs);
  current_threads = new task_thread *[number_of_procs];
  continue_this_thread = new bool[number_of_procs];
  abandon_thread = new bool[number_of_procs];
  idle_threads = new task_thread *[number_of_procs];

  for (uint32_t i = 0; i < number_of_procs; i++)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Initialising processor ", i, "\n");
    current_threads[i] = nullptr;
    continue_this_thread[i] = false;
    abandon_thread[i] = false;
    idle_threads[i] = nullptr;
  }

  KL_TRC_EXIT;
}

/// @brief Create a process to contain system-critical threads
///
/// This process contains idle threads for each processor, and a thread to handle the IRQ slowpath procedure.
///
/// @return The system process created here.
std::shared_ptr<task_process> task_create_system_process()
{
  KL_TRC_ENTRY;

  uint32_t number_of_procs = proc_mp_proc_count();
  std::shared_ptr<task_thread> new_idle_thread;
  std::shared_ptr<task_thread> irq_slowpath_thread;
  std::shared_ptr<task_process> system_process;
  mem_process_info *task0_mem_info;

  task0_mem_info = mem_task_get_task0_entry();
  ASSERT(task0_mem_info != nullptr);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Creating system process\n");
  system_process = task_process::create(proc_irq_slowpath_thread, true, task0_mem_info);
  ASSERT(system_process != nullptr);
  system_process->start_process();

  for (uint32_t i = 0; i < number_of_procs; i++)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Creating idle thread for processor", i, "\n");

    new_idle_thread = task_thread::create(task_idle_thread_cycle, system_process);
    new_idle_thread->stop_thread();
    ASSERT(new_idle_thread != nullptr);
    idle_threads[i] = new_idle_thread.get();
    task_thread_cycle_remove(idle_threads[i]);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "System process: ", system_process, "\n");
  KL_TRC_EXIT;

  return system_process;
}

void task_start_tasking()
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Beginning task switching\n");
  task_install_task_switcher();

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
task_thread *task_get_next_thread(bool abandon_this_thread)
{
  task_thread *next_thread = nullptr;
  task_thread *start_thread = nullptr;
  uint32_t proc_id;
  bool found_thread;
  KL_TRC_ENTRY;

  proc_id = proc_mp_this_proc_id();

  ASSERT(continue_this_thread != nullptr);
  ASSERT(current_threads != nullptr);

  if ((current_threads[proc_id] != nullptr) &&
      (abandon_this_thread == true))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Thread has requested its own destruction\n");

    current_threads[proc_id] = nullptr;
    continue_this_thread[proc_id] = false;
  }

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
      KL_TRC_TRACE(TRC_LVL::EXTRA, "Considering thread", (uint64_t)next_thread, "\n");
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
      KL_TRC_TRACE(TRC_LVL::EXTRA, "The next thread to execute is live thread", (uint64_t)next_thread, "\n");
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

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Next thread (addr)", (uint64_t)next_thread, "\n");
  KL_TRC_EXIT;

  return next_thread;
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

/// @brief Abandon this thread so it is never scheduled again.
///
/// When this thread is pre-empted by the scheduler no attempt is made to store any information from it into its thread
/// structure - indeed, the thread structure may have already been destroyed.
void task_abandon_this_thread()
{
  KL_TRC_ENTRY;

  abandon_thread[proc_mp_this_proc_id()] = true;

  KL_TRC_EXIT;
}

#ifdef AZALEA_TEST_CODE
void test_only_reset_task_mgr()
{
  KL_TRC_ENTRY;

  std::shared_ptr<task_process> system_proc;
  task_thread *idle_thread_to_del;

  if (idle_threads[0] != nullptr)
  {
    system_proc = idle_threads[0]->parent_process;
  }

  for (uint32_t i = 0; i < proc_mp_proc_count(); i++)
  {
    if (idle_threads[i] != nullptr)
    {
      idle_thread_to_del = idle_threads[i];
      task_thread_cycle_add(idle_thread_to_del);
      //idle_thread_to_del->destroy_thread();
      //delete idle_thread_to_del;
      //idle_threads[i] = nullptr;
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "All idle threads destroyed\n");

  if (system_proc != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Destroying system proc\n");
    system_proc->destroy_process();
  }

  delete[] current_threads;
  delete[] continue_this_thread;
  delete[] idle_threads;
  delete[] abandon_thread;

  current_threads = nullptr;
  continue_this_thread = nullptr;
  idle_threads = nullptr;
  start_of_thread_cycle = nullptr;

  thread_cycle_lock = 0;

  system_tree()->delete_child("proc");

  KL_TRC_EXIT;
}
#endif

/// @brief Add a new thread to the cycle of all threads
///
/// All threads are joined in a cycle by task_thread::next_thread. Add new_thread to this cycle.
///
/// @param new_thread The new thread to add.
void task_thread_cycle_add(task_thread *new_thread)
{
  KL_TRC_ENTRY;
  // We don't need to lock for the scheduler - the cycle is always in a consistent state as far as it's concerned,
  // but we do need to prevent two threads being added at once, since that might cause one or the other to be lost.
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
#ifndef _MSVC_LANG
    asm("hlt");
#else
    __halt();
#endif
  }
}
