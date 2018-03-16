/// @file Processor-generic functionality.
///
/// Some processor management functionality is common to all usual processor types, and that is handled in this file.

// Known defects:
// - Removing an IRQ handler just when an IRQ fires leads to a race condition in the list removal code that could,
//   potentially, cause some IRQ handlers to not fire on that occasion.
// - It's possible that removing an IRQ handler could cause crashes because we're not careful about just deleting the
//   item!
// - proc_irq_slowpath_thread has a pretty weak algorithm, and doesn't even attempt to sleep!

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include "devices/device_interface.h"

// It's a really bad idea to enable tracing in the live system!
//#define ENABLE_TRACING

namespace
{
  // This structure is used to store details about an individual IRQ handler.
  struct proc_irq_handler
  {
    // The receiver that should be called.
    IIrqReceiver *receiver;

    // Whether this receiver has requested the slow path, but not yet had the slow path executed.
    bool slow_path_reqd;
  };

  const unsigned char MAX_IRQ = 16;
  klib_list<proc_irq_handler *> irq_handlers[MAX_IRQ] = { { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr },
                                                          { nullptr, nullptr }
                                                          };
}

/// @brief Register an IRQ handler
///
/// Devices may request that they be invoked for a given IRQ by providing an IRQ Receiver. Details of receivers are
/// given in the documentation for `IIrqReceiver`.
///
/// @param irq_number The IRQ that the receiver wishes to handle.
///
/// @param receiver Pointer to an IRQ receiver that will be executed in response to the IRQ with the number given by
///                 `irq_number`
void proc_register_irq_handler(unsigned char irq_number, IIrqReceiver *receiver)
{
  KL_TRC_ENTRY;

  ASSERT(receiver != nullptr);
  ASSERT(irq_number < MAX_IRQ);

  klib_list_item<proc_irq_handler *> *new_item = new klib_list_item<proc_irq_handler *>;
  klib_list_item_initialize(new_item);

  proc_irq_handler *new_handler = new proc_irq_handler;
  new_handler->receiver = receiver;
  new_handler->slow_path_reqd = false;

  new_item->item = new_handler;

  klib_list_add_tail(&irq_handlers[irq_number], new_item);

  KL_TRC_EXIT;
}

/// @brief Unregister an IRQ handler
///
/// Stop sending IRQ events to this handler.
///
/// @param irq_number The IRQ that the receiver should no longer be called for.
///
/// @param receiver The receiver to unregister.
void proc_unregister_irq_handler(unsigned char irq_number, IIrqReceiver *receiver)
{
  KL_TRC_ENTRY;

  ASSERT(receiver != nullptr);
  ASSERT(irq_number < MAX_IRQ);
  ASSERT(!klib_list_is_empty(&irq_handlers[irq_number]));

  bool found_receiver = false;
  klib_list_item<proc_irq_handler *> *cur_item;
  proc_irq_handler *item;

  cur_item = irq_handlers[irq_number].head;

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

  KL_TRC_EXIT;
}

/// @brief The main IRQ handling code.
///
/// Called by the processor-specific code.
///
/// @param irq_number The number of the IRQ that fired.
void proc_handle_irq(unsigned char irq_number)
{
  KL_TRC_ENTRY;

  ASSERT(irq_number < MAX_IRQ);

  klib_list_item<proc_irq_handler *> *cur_item = irq_handlers[irq_number].head;
  proc_irq_handler *handler;

  while (cur_item != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Receiver: ", cur_item->item, "\n");
    handler = cur_item->item;
    ASSERT(handler != nullptr);

    if (handler->receiver->handle_irq_fast(irq_number))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Slow path requested\n");
      handler->slow_path_reqd = true;
    }

    cur_item = cur_item->next;
  }

  KL_TRC_EXIT;
}

/// @brief Iterates across all IRQ handlers to determine whether any of them have requested that the slow path be
///        handled.
///
/// If a slow IRQ handler is outstanding, it is called.
void proc_irq_slowpath_thread()
{
  klib_list_item<proc_irq_handler *> *cur_item;
  proc_irq_handler *item;

  while(1)
  {
    for (unsigned int i = 0; i < MAX_IRQ; i++)
    {
      cur_item = irq_handlers[i].head;

      while(cur_item != nullptr)
      {
        item = cur_item->item;
        ASSERT(item != nullptr);
        if (item->slow_path_reqd == true)
        {
          item->slow_path_reqd = false;
          item->receiver->handle_irq_slow(i);
        }

        cur_item = cur_item->next;
      }
    }
  }
}