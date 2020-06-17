/// @file
/// @brief Declarations internal to the processor/task-manager library.

#pragma once

#include <stdint.h>
#include "processor.h"

class proc_fs_root_branch;

/// @brief Stores details about an individual interrupt handler.
///
struct proc_interrupt_handler
{
  /// @brief The receiver that should be called.
  ///
  IInterruptReceiver *receiver;

  /// @brief Whether this receiver has requested the slow path, but not yet had the slow path executed.
  ///
  bool slow_path_reqd;
};

/// @brief Stores details for an individual interrupt number.
///
struct proc_interrupt_data
{
  bool reserved; ///< Has the interrupt number been reserved by the architecture, and is thus unavailable to drivers?

  bool is_irq; ///< Is this interrupt number actually an IRQ interrupt?

  klib_list<proc_interrupt_handler *> interrupt_handlers; ///< List of handlers for this interrupt.

  ipc::raw_spinlock list_lock; ///< Lock to protect interrupt_handlers.
};

void *task_int_create_exec_context(ENTRY_PROC entry_point,
                                   task_thread *new_thread,
                                   uint64_t param = 0,
                                   void *stack_ptr = nullptr);
void task_int_delete_exec_context(task_thread *old_thread);

task_thread *task_get_next_thread();

void task_install_task_switcher();
void task_platform_init();

// Interrupt handling:
// -------------------
void proc_config_interrupt_table();
extern "C" void proc_handle_interrupt(uint16_t interrupt_num);
extern "C" void proc_handle_irq(uint8_t irq_number);
void proc_interrupt_slowpath_thread();
void proc_tidyup_thread();

std::shared_ptr<task_process> task_create_system_process();

void task_thread_cycle_add(task_thread *new_thread);
void task_thread_cycle_remove(task_thread *thread);
void task_thread_cycle_lock();
void task_thread_cycle_unlock();
void task_idle_thread_cycle();

// These are documented elsewhere
/// @cond
extern const uint16_t PROC_NUM_INTERRUPTS;
extern const uint16_t PROC_IRQ_BASE;
extern const uint16_t PROC_NUM_IRQS;

extern proc_interrupt_data proc_interrupt_data_table[];

extern klib_list<std::shared_ptr<task_thread>> dead_thread_list;
extern std::atomic<task_process *> dead_processes;
/// @endcond
