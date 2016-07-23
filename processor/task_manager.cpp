//#define ENABLE_TRACING

#include "processor.h"
#include "processor-int.h"
#include "klib/klib.h"
#include "mem/mem.h"

// Martin's OS Task Manager.
//
// In the OS, the basic unit of execution is a thread. Multiple threads are grouped in to a process. A process defines
// the address space and permissions of all threads that are associated with it.

// This is a list of all processes known to the system.
klib_list process_list;

// This is a list of all threads known to the system.
klib_list complete_thread_list;

// For the purposes of the scheduler, which simply spins through all possible threads in turn, this is a list of
// all threads. The scheduler can then simply iterate over this list for ever and ever.
klib_list running_thread_list;

// TODO: Make this processor=specific (MT)
task_thread *current_thread = (task_thread *)NULL;

// TODO: Make this processor-specific (MT)
bool continue_this_thread;

// This field tracks which process ID to dispense next. It counts up until it reaches the maximum, then the system
// crashes.
// TODO: Prevent crashing. (STAB)
// TODO: Not thread safe, ironically. (MT) (LOCK)
PROCESS_ID next_proc_id;

// This field tracks which thread ID to dispense next. It counts up until it reaches the maximum, then the system
// crashes.
// TODO: Prevent crashing. (STAB)
// TODO: Not thread safe, ironically. (MT) (LOCK)
PROCESS_ID next_thread_id;


void task_gen_init(ENTRY_PROC kern_start_proc)
{
  KL_TRC_ENTRY;

  mem_process_info *task0_mem_info;
  task_process *first_process;

  next_proc_id = 0;
  next_thread_id = 0;
  continue_this_thread = false;

  klib_list_initialize(&process_list);
  klib_list_initialize(&complete_thread_list);
  klib_list_initialize(&running_thread_list);

  task0_mem_info = mem_task_get_task0_entry();
  ASSERT(task0_mem_info != NULL);

  KL_TRC_TRACE((TRC_LVL_FLOW, "Preparing the processor\n"));
  task_platform_init();

  KL_TRC_TRACE((TRC_LVL_FLOW, "Creating first process\n"));
  first_process = task_create_new_process(kern_start_proc, true, task0_mem_info);
  KL_TRC_DATA("First process at", (unsigned long)first_process);
  ASSERT(first_process != (task_process *)NULL);
  task_start_process(first_process);

  current_thread = (task_thread *)NULL;

  KL_TRC_TRACE((TRC_LVL_FLOW, "Beginning task switching\n"));
  task_install_task_switcher();

  KL_TRC_EXIT;
}

// Create a new process, with a thread starting at entry_point.
// If known, provide the memory manager information field now - this is only
// intended for use when creating the first kernel task (ID 0)
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
  if (mem_info != NULL)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "mem_info provided\n"));
    new_process->mem_info = mem_info;
  }
  else
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "No mem_info, create it\n"));
    new_process->mem_info = mem_task_create_task_entry();
  }
  new_process->process_id = next_proc_id;
  next_proc_id++;
  if (next_proc_id == 0)
  {
    panic("Out of process IDs!");
  }

  KL_TRC_TRACE((TRC_LVL_FLOW, "Checks complete, adding process to list\n"));
  klib_list_add_head(&process_list, &new_process->process_list_item);

  task_create_new_thread(entry_point, new_process);

  KL_TRC_EXIT;
  return new_process;
}

// Create a new thread starting at entry_point, with parent parent_process.
task_thread *task_create_new_thread(ENTRY_PROC entry_point, task_process *parent_process)
{
  KL_TRC_ENTRY;

  task_thread *new_thread = new task_thread;
  new_thread->parent_process = parent_process;

  klib_list_item_initialize(&new_thread->process_list_item);
  klib_list_item_initialize(&new_thread->master_thread_list_item);
  new_thread->process_list_item.item = (void *)new_thread;
  new_thread->master_thread_list_item.item = (void *)new_thread;
  klib_list_add_tail(&parent_process->child_threads, &new_thread->process_list_item);

  klib_list_item_initialize(&new_thread->running_thread_list_item);
  new_thread->running_thread_list_item.item = (void *)new_thread;

  new_thread->execution_context = task_int_create_exec_context(entry_point, new_thread);
  new_thread->thread_id = next_thread_id;
  next_thread_id++;
  if (next_thread_id == 0)
  {
    panic("Out of thread IDs!");
  }

  new_thread->running = false;

  klib_list_add_tail(&complete_thread_list, &new_thread->master_thread_list_item);

  KL_TRC_EXIT;
  return new_thread;
}

// Destroy a thread immediately.
void task_destroy_thread(task_thread *unlucky_thread)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE("Can't destroy threads.");
  KL_TRC_EXIT;
}

// Destroy a process (and by definition, all threads within it) immediately.
void task_destroy_process(task_process *unlucky_process)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE("Can't destroy processes.");
  KL_TRC_EXIT;
}

// Get the next thread to execute on this processor, and set it as the active one.
// TODO: What if no threads are ready to go? (STAB)
task_thread *task_get_next_thread()
{
  task_thread *next_thread;
  KL_TRC_ENTRY;

  if (continue_this_thread)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Requested to continue current thread\n"));
    next_thread = current_thread;
  }
  else if ((current_thread == NULL) || (current_thread->running_thread_list_item.next == NULL))
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Reached end of thread list, start over\n"));
    next_thread = (task_thread *)running_thread_list.head->item;
  }
  else
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Selecting next list item\n"));
    next_thread = (task_thread *)current_thread->running_thread_list_item.next->item;
  }

  current_thread = next_thread;

  KL_TRC_DATA("Next thread (addr)", (unsigned long)next_thread);
  KL_TRC_EXIT;

  return next_thread;
}

task_thread *task_get_cur_thread()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return current_thread;
}

THREAD_ID task_current_thread_id()
{
  KL_TRC_ENTRY;

  THREAD_ID ti = task_get_cur_thread()->thread_id;

  KL_TRC_TRACE((TRC_LVL_FLOW, "Returning thread ID: "));
  KL_TRC_TRACE((TRC_LVL_FLOW, ti));
  KL_TRC_TRACE((TRC_LVL_FLOW, "\n"));

  KL_TRC_EXIT;
  return ti;
}

PROCESS_ID task_current_process_id()
{
  KL_TRC_ENTRY;

  PROCESS_ID pi = task_get_cur_thread()->parent_process->process_id;

  KL_TRC_TRACE((TRC_LVL_FLOW, "Returning thread ID: "));
  KL_TRC_TRACE((TRC_LVL_FLOW, pi));
  KL_TRC_TRACE((TRC_LVL_FLOW, "\n"));

  KL_TRC_EXIT;
  return pi;
}

task_process *task_get_process_data(PROCESS_ID proc_id)
{
  panic("task_get_process_data not implemented");
  return (task_process *)NULL;
}

task_thread *task_get_thread_data(THREAD_ID thread_id)
{
  panic("task_get_thread_data not implemented");
  return (task_thread *)NULL;
}

// Set all threads in a process to be running
void task_start_process(task_process *process)
{
  KL_TRC_ENTRY;

  task_thread *next_thread = (task_thread *)NULL;
  klib_list_item *next_item = (klib_list_item *)NULL;

  KL_TRC_DATA("Process address", (unsigned long)process);

  ASSERT(process != (task_process*)NULL);
  next_item = process->child_threads.head;

  while (next_item != (klib_list_item *)NULL)
  {
    next_thread = (task_thread *)next_item->item;
    ASSERT(next_thread != NULL);
    KL_TRC_DATA("Next thread", (unsigned long)next_thread);
    task_start_thread(next_thread);

    next_item = next_item->next;
  }

  KL_TRC_EXIT;
}

// Stop all threads in a process
void task_stop_process(task_process *process)
{
  KL_TRC_ENTRY;

  task_thread *next_thread = (task_thread *)NULL;
  klib_list_item *next_item = (klib_list_item *)NULL;

  KL_TRC_DATA("Process address", (unsigned long)process);

  ASSERT(process != (task_process*)NULL);
  next_item = process->child_threads.head;

  while (next_item != (klib_list_item *)NULL)
  {
    next_thread = (task_thread *)next_item->item;
    ASSERT(next_thread != NULL);
    KL_TRC_DATA("Next thread", (unsigned long)next_thread);
    task_stop_thread(next_thread);

    next_item = next_item->next;
  }

  KL_TRC_EXIT;
}

// Start a specific thread
void task_start_thread(task_thread *thread)
{
  KL_TRC_ENTRY;
  KL_TRC_DATA("Thread address", (unsigned long)thread);

  ASSERT(thread != (task_thread *)NULL);
  if(!thread->running)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Thread not running, start it\n"));
    thread->running = true;
    klib_list_add_tail(&running_thread_list, &thread->running_thread_list_item);
  }

  KL_TRC_EXIT;
}

// Stop a specific thread
void task_stop_thread(task_thread *thread)
{
  KL_TRC_ENTRY;
  KL_TRC_DATA("Thread address", (unsigned long)thread);

  ASSERT(thread != (task_thread *)NULL);
  if(thread->running)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Thread running, stop it\n"));
    thread->running = false;
    klib_list_remove(&thread->running_thread_list_item);
  }

  KL_TRC_EXIT;
}

// Force the scheduler to continually re-schedule this thread. This is only really intended to be used by the kernel's
// synchronization code, to ensure that it can't be preempted in a state where it would be left in a deadlock.
// TODO: Update to support multiple processors! (MT)
void task_continue_this_thread()
{
  KL_TRC_ENTRY;

  continue_this_thread = true;

  KL_TRC_EXIT;
}

// Cancels the effect of task_continue_this_thread()
// TODO: Update to support multiple processors (MT)
void task_resume_scheduling()
{
  KL_TRC_ENTRY;

  continue_this_thread = false;

  KL_TRC_EXIT;
}

// Give up the rest of our time slice. In the simplest example, just HLT and wait for the scheduler to kick in.
void task_yield()
{
  KL_TRC_ENTRY;

  asm("hlt");

  KL_TRC_EXIT;
}
