#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include "klib/lists/lists.h"
#include "mem/mem.h"

// Main kernel interface to processor specific functions. Includes the task management system.

// Definition of a possible entry point:
typedef void (* ENTRY_PROC)();

typedef unsigned long PROCESS_ID;
typedef unsigned long THREAD_ID;

// Structure to hold information about a process. All information is stored here, to be accessed by the various
// components as needed. This removes the need for per-component lookup tables for each process.
struct task_process
{
    // A unique ID number for this process
    PROCESS_ID process_id;

    // Refer ourself back to the process list.
    klib_list_item process_list_item;

    // A list of all child threads.
    klib_list child_threads;

    // A pointer to the memory manager's information for this task.
    mem_process_info *mem_info;

    // Is the process running in kernel mode?
    bool kernel_mode;
};

// Structure to hold information about a thread.
struct task_thread
{
    // A unique ID number for this thread
    THREAD_ID thread_id;

    // This thread's parent process. The process defines the address space, permissions, etc.
    task_process *parent_process;

    // An entry for the parent's thread list.
    klib_list_item process_list_item;

    // An entry for the master thread list.
    klib_list_item master_thread_list_item;

    // An entry for the running thread list;
    klib_list_item running_thread_list_item;

    // A pointer to the thread's execution context. This is processor specific, so no specific structure can
    // be pointed to. Only processor-specific code should access this field.
    void *execution_context;

    // Is the thread running? It will only be considered for execution if so.
    bool running;
};

// Initialise the first processor and some of the data structures needed to manage all processors in the system.
void proc_gen_init();

// Continue initialization such that the other processors can be started, but leave them idle for now.
void proc_mp_init();

// Stop all processors other than the one this function is called on.
void proc_stop_other_procs();

// Stop the processor this function is called on. It may then be reinitialised
// later.
void proc_stop_this_proc();

// Stop all processors, including this one. The system will completely stop.
void proc_stop_all_procs();

// Stop interrupts on this processor. This should only be used in preparation
// for a panic.
void proc_stop_interrupts();

// Initialise the task management system and start it.
void task_gen_init(ENTRY_PROC kern_start_proc);

// Create a new process, with a thread starting at entry_point.
task_process *task_create_new_process(ENTRY_PROC entry_point,
    bool kernel_mode = false,
    mem_process_info *mem_info = (mem_process_info *)NULL);

// Create a new thread starting at entry_point, with parent parent_process.
task_thread *task_create_new_thread(ENTRY_PROC entry_point, task_process *parent_process);

// Destroy a thread immediately.
void task_destroy_thread(task_thread *unlucky_thread);

// Destroy a process (and by definition, all threads within it) immediately.
void task_destroy_process(task_process *unlucky_process);

// Return the current tasks' IDs.
THREAD_ID task_current_thread_id();
PROCESS_ID task_current_process_id();

// Return information about a specific task. This is intended to allow the various components to access their data,
// without having to store a parallel task list internally.
task_process *task_get_process_data(PROCESS_ID proc_id);
task_thread *task_get_thread_data(THREAD_ID thread_id);
task_thread *task_get_cur_thread();

// Start and stop threads and processes
void task_start_process(task_process *process);
void task_stop_process(task_process *process);
void task_start_thread(task_thread *thread);
void task_stop_thread(task_thread *thread);
void task_yield();

// Force the scheduler to re-schedule this thread continually, or allow it to schedule normally. This allows a thread
// to avoid being preempted in a state that might leave it in a deadlock. Naturally, it must be used with extreme care!
void task_continue_this_thread();
void task_resume_scheduling();

unsigned long proc_read_port(const unsigned long port_id, const unsigned char width);
void proc_write_port(const unsigned long port_id, const unsigned long value, const unsigned char width);

#endif /* PROCESSOR_H_ */
