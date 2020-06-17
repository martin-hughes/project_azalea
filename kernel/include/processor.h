/// @file
/// @brief Main processor control interface. Includes the task management system.

#pragma once

#include <stdint.h>

#include "mem.h"
#include "work_queue.h"

#include "types/thread.h"
#include "types/process.h"

#include <queue>


/// @brief Processor-specific information.
///
/// One of the processor-specific header files should typedef this struct with an appropriate template to create the
/// type processor_info.
struct processor_info
{
  /// A zero-based ID for the processor to be identified by. In the range 0 -> n-1, where n is the number of processors
  /// in the system
  uint32_t processor_id;

  /// Has the processor been started or not? That is, (in x64 speak) has it finished responding to the STARTUP IPI?
  volatile bool processor_running;
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

///////////////////////////////////////////////////////////////////////////////
// Architecture part                                                         //
///////////////////////////////////////////////////////////////////////////////

extern "C" uint64_t proc_read_port(const uint64_t port_id, const uint8_t width);
extern "C" void proc_write_port(const uint64_t port_id, const uint64_t value, const uint8_t width);

// Stop / start interrupts on this processor. It's not advisable for most code to call these functions, due to the
// performance impact.
void proc_stop_interrupts();
void proc_start_interrupts();

void proc_set_tls_register(TLS_REGISTERS reg, uint64_t value);

void proc_install_idt();

#ifdef AZALEA_TEST_CODE
void test_only_reset_task_mgr();
#endif
