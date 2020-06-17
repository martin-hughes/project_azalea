/// @file
/// @brief

#pragma once

#include "object_mgr.h"
#include "types/event.h"
#include "types/simple_types.h"
#include "types/list.h"

class task_process;


/// @brief Class to hold information about a thread.
///
/// At present, the thread class has no real internal logic. This is all delegated to function-based code in
/// task_manager.cpp as it comes from a very early point in the project.
///
/// task_thread derives from ipc::event, but doesn't change the default logic of that class. The event is signalled
/// when the thread is scheduled for destruction.
class task_thread : public virtual IHandledObject, public virtual ipc::event
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
  ipc::raw_spinlock cycle_lock;

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
