/// @file
/// @brief Main processor control interface.

#pragma once

#include <stdint.h>

#include "klib/data_structures/lists.h"
#include "klib/synch/kernel_locks.h"
#include "mem/mem.h"
#include "object_mgr/object_mgr.h"
#include "system_tree/system_tree_leaf.h"
#include "processor/synch_objects.h"
#include "processor/work_queue.h"

#include "devices/device_interface.h"

#include <queue>

// Main kernel interface to processor specific functions. Includes the task management system.

/// @brief Definition of a possible entry point:
typedef void (* ENTRY_PROC)();

// Forward declare task_thread since task_process and task_thread refer to each other in a cycle.
class task_thread;

/// Structure to hold information about a process. All information is stored here, to be accessed by the various
/// components as needed. This removes the need for per-component lookup tables for each process.
class task_process : public IHandledObject,
                     public WaitForFirstTriggerObject,
                     public std::enable_shared_from_this<task_process>,
                     public work::message_receiver
{
protected:
  task_process(ENTRY_PROC entry_point, bool kernel_mode = false, mem_process_info *mem_info = nullptr);

public:
  static std::shared_ptr<task_process> create(ENTRY_PROC entry_point,
                                              bool kernel_mode = false,
                                              mem_process_info *mem_info = nullptr);
  virtual ~task_process();

  void start_process();
  void stop_process();
  void destroy_process(uint64_t exit_code);

  virtual void handle_message(std::unique_ptr<msg::root_msg> &message) override;

protected:
  friend task_thread; ///< This is just a convenience really.
  void add_new_thread(std::shared_ptr<task_thread> new_thread);
  void thread_ending(task_thread *thread);

public:

  /// A list of all child threads.
  klib_list<std::shared_ptr<task_thread>> child_threads;

  /// A pointer to the memory manager's information for this task.
  mem_process_info *mem_info;

  /// Is the process running in kernel mode?
  bool kernel_mode;

  struct
  {
    /// Does this process accept messages? Messages can't be sent to the process unless this flag is true. Accepting
    /// messages is optional as not all processes will need the capability to receive messages.
    bool accepts_msgs{false};

    /// Lock to control the message queue.
    kernel_spinlock message_lock{0};

    /// Stores messages for retrieval by the process.
    std::queue<std::unique_ptr<msg::basic_msg>> message_queue;
  } messaging; ///< All variables related to the work queue/messaging system.

  /// Is this process currently being destroyed?
  bool being_destroyed;

  /// Has this process ever been started?
  bool has_ever_started;

  /// Store handles and the objects they correlate to.
  object_manager proc_handles;

  kernel_spinlock map_ops_lock{0}; ///< Lock protecting the futex map, below.

  std::map<uint64_t, std::vector<task_thread *>> futex_map; ///< Map of all futexes waiting in this process.

  uint64_t exit_code{0}; ///< Code provided when the process is exiting.

  OPER_STATUS proc_status{OPER_STATUS::OK}; ///< Current process status. Only OK, STOPPED and FAILED are valid.

  /// @brief Add this process to the list of dead processes maintained in processor.cpp
  ///
  /// The process is then destroyed asynchronously. Other, synchronous, destruction attempts are inhibited after this
  /// function is called.
  void add_to_dead_list();

  /// @brief Points to another process that has died.
  ///
  /// This pointer is used to form a stack of processes that have died due to hitting an exception handler. They are
  /// then tidied by proc_tidyup_thread. This stack is pushed by an exception handler, and popped by
  /// proc_tidyup_thread.
  task_process *next_defunct_process{nullptr};

  /// @brief Prevent this process being destroyed if it's in the dead thread list.
  ///
  /// This flag is set immediately before adding this process to the defunct process list. If a thread attempts to
  /// destroy the process while this flag is set then the attempt is ignored - this means pointers in the defunct
  /// process list will always be valid.
  bool in_dead_list{false};
};

/// @brief Class to hold information about a thread.
///
/// At present, the thread class has no real internal logic. This is all delegated to function-based code in
/// task_manager.cpp as it comes from a very early point in the project.
///
/// task_thread derives from WaitObject, but doesn't change the default logic of that class. The WaitObject is
/// signalled when the thread is scheduled for destruction.
class task_thread : public IHandledObject, public WaitObject
{
protected:
  task_thread(ENTRY_PROC entry_point, std::shared_ptr<task_process> parent, uint64_t param, void *stack_ptr);

public:
  static std::shared_ptr<task_thread> create(ENTRY_PROC entry_point,
                                             std::shared_ptr<task_process> parent,
                                             uint64_t param = 0,
                                             void *stack_ptr = nullptr);
  virtual ~task_thread();

  bool start_thread();
  bool stop_thread();
  void destroy_thread();

  /// A pointer to the next thread. In normal operation, these form a cycle of threads, and the task manager is able
  /// to manipulate this cycle without breaking the chain.
  task_thread *next_thread;

  /// A lock used by the task manager to claim ownership of this thread. It has several meanings:
  /// - The task manager might be about to manipulate the thread cycle, so the scheduler should avoid scheduling this
  ///   thread
  /// - The scheduler might be running this thread, in which case no other processor should run it as well
  kernel_spinlock cycle_lock;

  /// Is the thread running? It will only be considered for execution if so.
  volatile bool permit_running;

  /// This thread's parent process. The process defines the address space, permissions, etc.
  std::shared_ptr<task_process> parent_process;

  /// An entry for the parent's thread list.
  klib_list_item<std::shared_ptr<task_thread>> *process_list_item;

  /// A pointer to the thread's execution context. This is processor specific, so no specific structure can
  /// be pointed to. Only processor-specific code should access this field.
  void *execution_context;

  /// This item is used to associate the thread with the list of threads waiting for a mutex, semaphore or other
  /// synchronization primitive. The list itself is owned by that primitive, but this item must be initialized with the
  /// rest of this structure.
  klib_list_item<std::shared_ptr<task_thread>> *synch_list_item;

  /// Has the thread been destroyed? Various operations are not permitted on a destroyed thread. This object will
  /// continue to exist until all references to it have been released.
  bool thread_destroyed;

  /// Is this a work queue worker thread? Knowing this allows us to spin up another thread if a worker thread is about
  /// to block waiting for another work item to finish. The work queue system will endeavour to maintain the minimum
  /// possible number of active threads, so if this thread is a work queue thread then it may be stopped after this
  /// work item completes.
  bool is_worker_thread;

  /// If this value is set to non-zero, and the thread is sleeping, and the system timer is greater than this value,
  /// then the scheduler will wake this thread and start it running again. This is an absolute value in nanoseconds.
  uint64_t wake_thread_after{0};

  /// The number of TLS slots provided per thread in the kernel.
  static const uint8_t MAX_TLS_KEY = 16;

  /// @brief Slots for thread local storage.
  ///
  /// These slots are for thread local storage within the kernel only. User-mode thread local storage is dealt with in
  /// user-mode by the user's preferred library.
  void *thread_local_storage_slot[MAX_TLS_KEY];

#ifdef AZALEA_TEST_CODE
  friend void test_only_reset_task_mgr();
#endif
};

/// @brief Processor-specific information.
///
/// One of the processor-specific header files should typedef this struct with an appropriate template to create the
/// type processor_info.
template <typename T> struct processor_info_generic
{
  /// A zero-based ID for the processor to be identified by. In the range 0 -> n-1, where n is the number of processors
  /// in the system
  uint32_t processor_id;

  /// Has the processor been started or not? That is, (in x64 speak) has it finished responding to the STARTUP IPI?
  volatile bool processor_running;

  /// Platform specific processor information
  T platform_data;
};

/// Possible messages to signal between processes
enum class PROC_IPI_MSGS
{
  RESUME,          ///< Bring the processor back in to action after suspending it.
  SUSPEND,         ///< Halt the processor with interrupts disabled.
  TLB_SHOOTDOWN,   ///< Invalidate the processor's page tables.
  RELOAD_IDT,      ///< Pick up changes to the system IDT.
};

// Initialise the first processor and some of the data structures needed to manage all processors in the system.
void proc_gen_init();

// Continue initialisation such that the other processors can be started, but leave them idle for now.
void proc_mp_init();

// Start all APs.
void proc_mp_start_aps();

// Stop the processor this function is called on. It may then be reinitialised later.
void proc_stop_this_proc();

// Stop all other processors except this one.
void proc_stop_other_procs();

// Stop all processors, including this one. The system will completely stop.
void proc_stop_all_procs();

// Stop / start interrupts on this processor. It's not advisable for most code to call these functions, due to the
// performance impact.
void proc_stop_interrupts();
void proc_start_interrupts();

// Initialise the task management system.
std::shared_ptr<task_process> task_init();
void task_gen_init();

// Begin multi-tasking
void task_start_tasking();

void task_set_start_params(task_process * process, uint64_t argc, char **argv, char **env);

// Return information about a specific task. This is intended to allow the various components to access their data,
// without having to store a parallel task list internally.
task_thread *task_get_cur_thread();

// Force a reschedule on this processor.
void task_yield();

// Multiple processor control functions
uint32_t proc_mp_proc_count();
uint32_t proc_mp_this_proc_id();
void proc_mp_signal_processor(uint32_t proc_id, PROC_IPI_MSGS msg, bool must_complete);
void proc_mp_signal_all_processors(PROC_IPI_MSGS msg, bool exclude_self, bool wait_for_complete);
void proc_mp_receive_signal(PROC_IPI_MSGS msg);

// Force the scheduler to re-schedule this thread continually, or allow it to schedule normally. This allows a thread
// to avoid being preempted in a state that might leave it in a deadlock. Naturally, it must be used with extreme care!
void task_continue_this_thread();
void task_resume_scheduling();

uint64_t proc_read_port(const uint64_t port_id, const uint8_t width);
void proc_write_port(const uint64_t port_id, const uint64_t value, const uint8_t width);

// Allow drivers to handle IRQs and normal interrupts.
class IInterruptReceiver; // defined in device_interface.h
void proc_register_irq_handler(uint8_t irq_number, IInterruptReceiver *receiver);
void proc_unregister_irq_handler(uint8_t irq_number, IInterruptReceiver *receiver);
void proc_register_interrupt_handler(uint8_t interrupt_number, IInterruptReceiver *receiver);
void proc_unregister_interrupt_handler(uint8_t interrupt_number, IInterruptReceiver *receiver);

bool proc_request_interrupt_block(uint8_t num_interrupts, uint8_t &start_vector);

// Stack control functions
void *proc_allocate_stack(bool kernel_mode, task_process *proc = nullptr);
void proc_deallocate_stack(void *stack_ptr);

#ifdef AZALEA_TEST_CODE
void test_only_reset_task_mgr();
#endif
