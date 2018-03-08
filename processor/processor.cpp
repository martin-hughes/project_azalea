/// @file Processor-generic functionality.
///
/// Some processor management functionality is common to all usual processor types, and that is handled in this file.

// Known defects:
// - Removing an IRQ handler just when an IRQ fires leads to a race condition in the list removal code that could,
//   potentially, cause some IRQ handlers to not fire on that occasion.

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include "devices/device_interface.h"

namespace
{
  const unsigned char MAX_IRQ = 16;
  klib_list irq_handlers[MAX_IRQ] = { { nullptr, nullptr },
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

void proc_register_irq_handler(unsigned char irq_number, IIrqReceiver *receiver)
{
  KL_TRC_ENTRY;

  ASSERT(receiver != nullptr);
  ASSERT(irq_number < MAX_IRQ);

  klib_list_item *new_item = new klib_list_item;
  klib_list_item_initialize(new_item);
  new_item->item = reinterpret_cast<void *>(receiver);

  klib_list_add_tail(&irq_handlers[irq_number], new_item);

  KL_TRC_EXIT;
}

void proc_unregister_irq_handler(unsigned char irq_number, IIrqReceiver *receiver)
{
  KL_TRC_ENTRY;

  ASSERT(receiver != nullptr);
  ASSERT(irq_number < MAX_IRQ);
  ASSERT(!klib_list_is_empty(&irq_handlers[irq_number]));

  bool found_receiver = false;
  klib_list_item *cur_item;
  void *recv_ptr = reinterpret_cast<void *>(receiver);

  cur_item = irq_handlers[irq_number].head;

  while(cur_item != nullptr)
  {
    if (cur_item->item == recv_ptr)
    {
      found_receiver = true;
      klib_list_remove(cur_item);
      delete cur_item;
      cur_item = nullptr;

      break;
    }

    cur_item = cur_item->next;
  }

  ASSERT(found_receiver);

  KL_TRC_EXIT;
}

void proc_handle_irq(unsigned char irq_number)
{
  KL_TRC_ENTRY;

  ASSERT(irq_number < MAX_IRQ);

  klib_list_item *cur_item = irq_handlers[irq_number].head;
  IIrqReceiver *receiver;

  while (cur_item != nullptr)
  {
    receiver = reinterpret_cast<IIrqReceiver *>(cur_item->item);
    ASSERT(receiver != nullptr);

    // A receiver returning false indicates that it doesn't want any other IRQ handler to be executed, for some reason.
    if (!receiver->handle_irq(irq_number))
    {
      break;
    }

    cur_item = cur_item->next;
  }

  KL_TRC_EXIT;
}