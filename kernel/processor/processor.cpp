/// @file
/// @brief Processor-generic functionality.
///
/// Some processor management functionality is common to all usual processor types, and that is handled in this file.

// Known defects:
// - Removing an IRQ handler just when an IRQ fires leads to a race condition in the list removal code that could,
//   potentially, cause some IRQ handlers to not fire on that occasion.
// - It's possible that removing an IRQ handler could cause crashes because we're not careful about just deleting the
//   item!
// - proc_irq_slowpath_thread has a pretty weak algorithm, and doesn't even attempt to sleep!
// - The list for removing dead threads isn't locked or protected in any way at all...
// - There should be a better waiting algorithm for proc_tidyup_thread.

// It's a really bad idea to enable tracing in the live system!
//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include "devices/device_interface.h"
#include "processor/timing/timing.h"

namespace
{
  bool interrupt_table_cfgd = false;
}

/// @brief A list of dead threads still to be tidied.
///
/// Dead threads are those that are scheduled to exit. Structures associated with them are destroyed asynchronously,
/// and this is a list of threads that still need to be destroyed.
klib_list<std::shared_ptr<task_thread>> dead_thread_list;

/// @brief Beginning of a stack of processes to destroy.
///
/// This pointer is the head of a stack of processes that have hit an unhandled exception handler and need to be
/// destroyed. The dead process objects point to the next dead process using task_process::next_defunct_process.
///
/// The stack is pushed by the exception handlers, and popped by proc_tidyup_thread.
///
/// These pointers cannot become stale, because until the process is destroyed the pointer will be valid, even if
/// another thread tries to destroy the process. This is helped by the task_process::in_dead_list flag.
std::atomic<task_process *> dead_processes;

/// @brief Configure the kernel's interrupt data table.
///
/// Note that this is not the same as the system IDT. The IDT tells the processor where to execute code when an
/// interrupt begins, the kernel then looks in this table to determine which objects have interrupt handlers to
/// execute.
void proc_config_interrupt_table()
{
  KL_TRC_ENTRY;

  // We only want to execute this function once. This isn't perfect locking, but it'll do - this function gets called
  // very early on in the setup process, well before any multi-tasking, so any mistaken calls later on will definitely
  // get caught by this assert.
  ASSERT(interrupt_table_cfgd == false);
  interrupt_table_cfgd = true;

  for (int i = 0; i < PROC_NUM_INTERRUPTS; i++)
  {
    klib_list_initialize(&proc_interrupt_data_table[i].interrupt_handlers);
    proc_interrupt_data_table[i].reserved = false;
    proc_interrupt_data_table[i].is_irq = false;
    klib_synch_spinlock_init(proc_interrupt_data_table[i].list_lock);
  }

  KL_TRC_EXIT;
}

/// @brief Register an Interrupt handler
///
/// Devices may request that they be invoked for a given interrupt by providing an interrupt Receiver. Details of
/// receivers are given in the documentation for `IInterruptReceiver`.
///
/// @param interrupt_number The interrupt that the receiver wishes to handle.
///
/// @param receiver Pointer to an interrupt receiver that will be executed in response to the interrupt with the number
///                 given by `interrupt_number`
void proc_register_interrupt_handler(uint8_t interrupt_number, IInterruptReceiver *receiver)
{

  KL_TRC_ENTRY;

  ASSERT(receiver != nullptr);
  ASSERT(interrupt_table_cfgd);
  ASSERT(interrupt_number < PROC_NUM_INTERRUPTS);

  // Don't allow an attempt to register a handler for a system-reserved interrupt unless it's to register a handler for
  // an IRQ.
  ASSERT((proc_interrupt_data_table[interrupt_number].reserved == false) ||
         (proc_interrupt_data_table[interrupt_number].is_irq == true));


  klib_synch_spinlock_lock(proc_interrupt_data_table[interrupt_number].list_lock);
  klib_list_item<proc_interrupt_handler *> *new_item = new klib_list_item<proc_interrupt_handler *>;
  klib_list_item_initialize(new_item);

  proc_interrupt_handler *new_handler = new proc_interrupt_handler;
  new_handler->receiver = receiver;
  new_handler->slow_path_reqd = false;

  new_item->item = new_handler;

  klib_list_add_tail(&proc_interrupt_data_table[interrupt_number].interrupt_handlers, new_item);
  klib_synch_spinlock_unlock(proc_interrupt_data_table[interrupt_number].list_lock);

  KL_TRC_EXIT;
}

/// @brief Unregister an interrupt handler
///
/// Stop sending interrupt events to this handler.
///
/// @param interrupt_number The interrupt that the receiver should no longer be called for.
///
/// @param receiver The receiver to unregister.
void proc_unregister_interrupt_handler(uint8_t interrupt_number, IInterruptReceiver *receiver)
{
  KL_TRC_ENTRY;

  ASSERT(receiver != nullptr);
  ASSERT(interrupt_table_cfgd);

  // Don't allow an attempt to unregister a handler for a system-reserved interrupt unless it's to register a handler
  // for an IRQ.
  ASSERT((proc_interrupt_data_table[interrupt_number].reserved == false) ||
         (proc_interrupt_data_table[interrupt_number].is_irq == true));

  klib_synch_spinlock_lock(proc_interrupt_data_table[interrupt_number].list_lock);
  ASSERT(!klib_list_is_empty(&proc_interrupt_data_table[interrupt_number].interrupt_handlers));

  bool found_receiver = false;
  klib_list_item<proc_interrupt_handler *> *cur_item;
  proc_interrupt_handler *item;

  cur_item = proc_interrupt_data_table[interrupt_number].interrupt_handlers.head;

  while(cur_item != nullptr)
  {
    item = cur_item->item;
    ASSERT(item != nullptr);
    if (item->receiver == receiver)
    {
      found_receiver = true;
      klib_list_remove(cur_item);
      delete cur_item;
      cur_item = nullptr;

      delete item;
      item = nullptr;

      break;
    }

    cur_item = cur_item->next;
  }

  ASSERT(found_receiver);
  klib_synch_spinlock_unlock(proc_interrupt_data_table[interrupt_number].list_lock);

  KL_TRC_EXIT;
}

/// @brief Register an IRQ handler
///
/// Devices may request that they be invoked for a given IRQ by providing an interrupt Receiver. Details of receivers
/// are given in the documentation for `IInterruptReceiver`. When the receiver is invoked, it will be given the number
/// of the IRQ rather than the underlying interrupt number.
///
/// @param irq_number The IRQ that the receiver wishes to handle.
///
/// @param receiver Pointer to an interrupt receiver that will be executed in response to the IRQ with the number given
///                 by `irq_number`
void proc_register_irq_handler(uint8_t irq_number, IInterruptReceiver *receiver)
{
  KL_TRC_ENTRY;

  ASSERT(irq_number < PROC_NUM_IRQS);

  proc_register_interrupt_handler(irq_number + PROC_IRQ_BASE, receiver);

  KL_TRC_EXIT;
}

/// @brief Unregister an IRQ handler
///
/// Stop sending IRQ events to this handler.
///
/// @param irq_number The IRQ that the receiver should no longer be called for.
///
/// @param receiver The receiver to unregister.
void proc_unregister_irq_handler(uint8_t irq_number, IInterruptReceiver *receiver)
{
  KL_TRC_ENTRY;

  ASSERT(irq_number < PROC_NUM_IRQS);

  proc_unregister_interrupt_handler(irq_number + PROC_IRQ_BASE, receiver);

  KL_TRC_EXIT;
}

/// @brief Request a contiguous set of interrupt vector numbers for a driver to use.
///
/// If it is able to, this function will allocate a block of interrupt vectors of the requested size, returning the
/// first vector in `start_vector`. The block of allocated vectors is contiguous, finishing with the last vector at
/// `start_vector + num_interrupts - 1`. These interrupts may be shared with other drivers.
///
/// `start_vector` will be aligned on an integer multiple of num_interrupts, if necessary rounded up to the next power
/// of two.
///
/// @param[in] num_interrupts How many interrupts the caller requests. Need not be a power of two. Maximum of 32.
///
/// @param[out] start_vector The first vector in the allocated block. Will always be on an integer multiple of
///             num_interrupts (rounded up to the next power of two if necessary).
///
/// @return True if the request could be fulfilled and interrupts allocated. False if, for some reason, the requested
///         block of interrupts could not be allocated.
bool proc_request_interrupt_block(uint8_t num_interrupts, uint8_t &start_vector)
{
  uint16_t rounded_num_ints;
  bool result = true;

  KL_TRC_ENTRY;

  // For the time being, make no attempt to even try and shared out interrupts. That will be a later performance
  //improvement.

  rounded_num_ints = round_to_power_two(num_interrupts);

  if (rounded_num_ints > 32)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Too many interrupts\n");
    result = false;
  }
  else
  {
    // To clear the processor and IRQ areas.
    start_vector = 64;
  }

  KL_TRC_EXIT;
  return result;
}

/// @brief The main interrupt handling code.
///
/// Called by the processor-specific code.
///
/// @param interrupt_number The number of the IRQ that fired.
void proc_handle_interrupt(uint16_t interrupt_number)
{
  KL_TRC_ENTRY;

  ASSERT(interrupt_number < PROC_NUM_INTERRUPTS);
  ASSERT(interrupt_table_cfgd);

  klib_list_item<proc_interrupt_handler *> *cur_item =
    proc_interrupt_data_table[interrupt_number].interrupt_handlers.head;
  proc_interrupt_handler *handler;

  while (cur_item != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Receiver: ", cur_item->item, "\n");
    handler = cur_item->item;
    ASSERT(handler != nullptr);

    if (handler->receiver->handle_interrupt_fast(interrupt_number))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Slow path requested\n");
      handler->slow_path_reqd = true;
    }

    cur_item = cur_item->next;
  }

  KL_TRC_EXIT;
}

/// @brief The main IRQ handling code.
///
/// Called by the processor-specific code.
///
/// @param irq_number The number of the IRQ that fired.
void proc_handle_irq(uint8_t irq_number)
{
  uint16_t interrupt_number = irq_number + PROC_IRQ_BASE;

  KL_TRC_ENTRY;

  ASSERT(irq_number < PROC_NUM_IRQS);
  ASSERT(interrupt_table_cfgd);

  klib_list_item<proc_interrupt_handler *> *cur_item =
    proc_interrupt_data_table[interrupt_number].interrupt_handlers.head;
  proc_interrupt_handler *handler;

  while (cur_item != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Receiver: ", cur_item->item, "\n");
    handler = cur_item->item;
    ASSERT(handler != nullptr);

    if (handler->receiver->handle_interrupt_fast(irq_number))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Slow path requested\n");
      handler->slow_path_reqd = true;
    }

    cur_item = cur_item->next;
  }

  KL_TRC_EXIT;
}

/// @brief Iterates across all interrupt handlers to determine whether any of them have requested that the slow path be
///        handled.
///
/// If a slow interrupt handler is outstanding, it is called.
void proc_interrupt_slowpath_thread()
{
  klib_list_item<proc_interrupt_handler *> *cur_item;
  proc_interrupt_handler *item;
  ASSERT(interrupt_table_cfgd);
  uint32_t interrupt_num;

  task_get_cur_thread()->is_worker_thread = true;

  while(1)
  {
    for (uint32_t i = 0; i < PROC_NUM_INTERRUPTS; i++)
    {
      klib_synch_spinlock_lock(proc_interrupt_data_table[i].list_lock);
      cur_item = proc_interrupt_data_table[i].interrupt_handlers.head;
      interrupt_num = i;

      if (proc_interrupt_data_table[i].is_irq)
      {
        interrupt_num -= PROC_IRQ_BASE;
      }

      while(cur_item != nullptr)
      {
        item = cur_item->item;
        ASSERT(item != nullptr);
        if (item->slow_path_reqd == true)
        {
          item->slow_path_reqd = false;
          item->receiver->handle_interrupt_slow(interrupt_num);
        }

        cur_item = cur_item->next;
      }
      klib_synch_spinlock_unlock(proc_interrupt_data_table[i].list_lock);
    }
  }
}

/// @brief Runs tidy-up tasks that can't be run in the context of other threads.
///
/// At the moment, this is only destroying the thread objects of threads that terminate themselves - since trying to
/// delete thread objects from within the actual thread could lead to deadlock.
void proc_tidyup_thread()
{
  std::shared_ptr<task_thread> dead_thread;
  task_process *next_process;

  while(1)
  {
    // Handle dead threads.
    while(!klib_list_is_empty(&dead_thread_list))
    {
      dead_thread = dead_thread_list.head->item;
      klib_list_remove(dead_thread->synch_list_item);
      dead_thread->synch_list_item->item = nullptr;
      if (dead_thread.unique())
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Delete thread\n");
        dead_thread = nullptr;
      }
    }

    // Handles dead processes
    while ((next_process = dead_processes.load()))
    {
      if (std::atomic_compare_exchange_weak(&dead_processes, &next_process, next_process->next_defunct_process))
      {
        kl_trc_trace(TRC_LVL::FLOW, "Destroy process ", next_process, "\n");
        next_process->destroy_process(next_process->exit_code);
      }
    }

    task_yield();

    time_stall_process(1000000000);
  }
}
