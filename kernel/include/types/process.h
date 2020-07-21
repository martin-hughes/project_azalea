/// @file
/// @brief

#pragma once

#include "object_mgr.h"
#include "types/event.h"
#include "types/list.h"
#include "types/spinlock.h"
#include "types/simple_types.h"
#include "types/system_tree_simple_branch.h"

#include <queue>
#include <vector>

class task_thread;

/// Structure to hold information about a process. All information is stored here, to be accessed by the various
/// components as needed. This removes the need for per-component lookup tables for each process.
class task_process : public virtual system_tree_simple_branch,
                     public virtual ipc::event,
                     public std::enable_shared_from_this<task_process>,
                     public virtual work::message_receiver
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

  virtual void handle_message(std::unique_ptr<msg::root_msg> message) override;

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
    ipc::raw_spinlock message_lock{0};

    /// Stores messages for retrieval by the process.
    std::queue<std::unique_ptr<msg::basic_msg>> message_queue;
  } messaging; ///< All variables related to the work queue/messaging system.

  /// Is this process currently being destroyed?
  bool being_destroyed;

  /// Has this process ever been started?
  bool has_ever_started;

  /// Store handles and the objects they correlate to.
  object_manager proc_handles;

  ipc::raw_spinlock map_ops_lock{0}; ///< Lock protecting the futex map, below.

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
