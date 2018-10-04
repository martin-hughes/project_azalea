/// @file
/// @brief Implements xHCI rings.

// Known deficiencies - we make no attempt to check rings don't cross 64kB boundaries.

//#define ENABLE_TRACING

#include "devices/usb/controllers/usb_xhci_controller.h"
#include "processor/timing/timing.h"

#include "klib/klib.h"

using namespace usb::xhci;

//----------------------------
// Generic ring management.
//----------------------------

namespace usb { namespace xhci
{
  /// @brief Copy TRBs from one place to another.
  ///
  /// We use this rather than memcpy to ensure 64-bit copies in the correct order for the xHCI.
  ///
  /// @param dest The destination of the copy.
  ///
  /// @parm src The source of the copy.
  void copy_trb(basic_trb *dest, basic_trb *src)
  {
    dest->reserved_1 = src->reserved_1;
    dest->reserved_2 = src->reserved_2;
  }
} }

/// @brief Create a new TRB ring.
///
/// @param max_entries The number of entries to create the ring with.
trb_generic_ring::trb_generic_ring(uint16_t max_entries)
{
  uint64_t addr_num;
  uint64_t offset;
  link_trb *final_trb;

  KL_TRC_ENTRY;

  // There has to be at least 2 entries, so the final entry can be a link TRB and still have one TRB that isn't a link.
  // I'm not sure that a 1-entry Ring is that useful though!
  ASSERT(max_entries > 1);

  // TRB rings must start on a 16-byte boundary, but our allocator does not guarantee alignment. Allocate sufficient
  // space for the ring and do the alignment manually. Since TRBs are 16 bytes long, just allocate space for one more
  // than requested and there'll be space for the right number.
  trb_allocation = kmalloc((max_entries + 1) * sizeof(basic_trb));
  ASSERT(trb_allocation != nullptr);

  addr_num = reinterpret_cast<uint64_t>(trb_allocation);
  offset = addr_num % 16;
  if (offset != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Realigning allocation.");
    addr_num = addr_num + 16 - offset;
  }

  trb_ring_base = reinterpret_cast<basic_trb *>(addr_num);
  enqueue_ptr = reinterpret_cast<template_trb *>(trb_ring_base);

  for (int i = 0; i < max_entries - 1; i++)
  {
    trb_ring_base[i].populate();
  }

  final_trb = reinterpret_cast<link_trb *>(&trb_ring_base[max_entries - 1]);
  final_trb->populate(trb_ring_base, 0, 0, 0, 0, 0);

  // Calculate the ring's physical address and store it for later.
  trb_ring_base_phys = mem_get_phys_addr(trb_ring_base);

  KL_TRC_EXIT;
}

/// @brief Tidies up the ring.
///
trb_generic_ring::~trb_generic_ring()
{
  // We also need to make sure the controller isn't actually using the ring...
  INCOMPLETE_CODE("~trb_ring");

  kfree(trb_allocation);
}

/// @brief Get the physical base address of the first TRB, so it can be given to the controller.
///
/// @return The physical address of the first TRB.
void *trb_generic_ring::get_phys_base_address()
{
  return trb_ring_base_phys;
}

/// @brief Queue a TRB at the enqueue pointer.
///
/// @param new_trb Pointer the TRB to queue.
///
/// @return True if the TRB could be queued. False otherwise.
bool trb_generic_ring::queue_trb(basic_trb *new_trb)
{
  bool result = false;
  template_trb *trb_template = reinterpret_cast<template_trb *>(new_trb);

  KL_TRC_ENTRY;

  if (new_trb == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "TRB nullptr\n");
    result = false;
  }
  else if (!is_valid_trb(*trb_template))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid TRB for this ring type\n");
    result = false;
  }
  else if (enqueue_ptr->cycle == 1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Ring is full\n");
    return false;
  }
  else
  {
    // Ensure template TRB cycle bit is 0, to avoid confusion.
    trb_template->cycle = 0;

    // Copy the TRB.
    copy_trb(reinterpret_cast<basic_trb *>(enqueue_ptr), reinterpret_cast<basic_trb *>(trb_template));

    // Advance the enqueue pointer. For a simplification, we know that our ring only contains one link TRB that points
    // back to the start.
    enqueue_ptr++;
    if (enqueue_ptr->trb_type == TRB_TYPES::LINK)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Returning to beginning of ring\n");
      enqueue_ptr = reinterpret_cast<template_trb *>(trb_ring_base);
    }

    result = true;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

//---------------------------------------
// Transfer rings
//---------------------------------------
trb_transfer_ring::trb_transfer_ring(uint16_t max_entries) : trb_generic_ring(max_entries)
{
  // No particular actions.
}

/// @brief Is this TRB valid for queueing within this ring type?
///
/// Valid TRB types for transfer rings are given in the xHCI spec.
///
/// @param new_trb The TRB to examine.
///
/// @return True if the TRB is valid on a transfer ring, false otherwise.
bool trb_transfer_ring::is_valid_trb(template_trb &new_trb)
{
  bool result = false;

  KL_TRC_ENTRY;

  // Link TRBs are missing from this list. We don't allow link TRBs to be added, because that would allow the
  // enqueue-er to break our ring.
  switch(new_trb.trb_type)
  {
  case TRB_TYPES::NORMAL:
  case TRB_TYPES::SETUP_STAGE:
  case TRB_TYPES::DATA_STAGE:
  case TRB_TYPES::STATUS_STAGE:
  case TRB_TYPES::ISOCH:
  case TRB_TYPES::EVENT_DATA:
  case TRB_TYPES::NO_OP:
    result = true;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "TRB type: ", new_trb.trb_type, ", result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}

//---------------------------------------
// Command rings
//---------------------------------------

trb_command_ring::trb_command_ring(uint16_t max_entries) : trb_generic_ring(max_entries)
{
  // No additional actions yet.
}

/// @brief Is this TRB valid for queueing within this ring type?
///
/// Valid TRB types for command rings are given in the xHCI spec.
///
/// @param new_trb The TRB to examine.
///
/// @return True if the TRB is valid on a command ring, false otherwise.
bool trb_command_ring::is_valid_trb(template_trb &new_trb)
{
  bool result = false;

  KL_TRC_ENTRY;

  // Link TRBs are missing from this list. We don't allow link TRBs to be added, because that would allow the
  // enqueue-er to break our ring.
  switch(new_trb.trb_type)
  {
  case TRB_TYPES::ENABLE_SLOT_CMD:
  case TRB_TYPES::DISABLE_SLOT_CMD:
  case TRB_TYPES::ADDRESS_DEVICE_CMD:
  case TRB_TYPES::CONFIG_ENDPOINT_CMD:
  case TRB_TYPES::EVAL_CONTEXT_CMD:
  case TRB_TYPES::RESET_ENDPOINT_CMD:
  case TRB_TYPES::STOP_ENDPOINT_CMD:
  case TRB_TYPES::SET_TR_DEQUEUE_PTR_CMD:
  case TRB_TYPES::RESET_DEVICE_CMD:
  case TRB_TYPES::FORCE_EVENT_CMD:
  case TRB_TYPES::NEGOTIATE_BANDWIDTH_CMD:
  case TRB_TYPES::SET_LATENCY_TOL_CMD:
  case TRB_TYPES::GET_PORT_BANDWIDTH_CMD:
  case TRB_TYPES::FORCE_HEADER_CMD:
  case TRB_TYPES::NO_OP_CMD:
    result = true;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "TRB type: ", new_trb.trb_type, ", result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}

//---------------------------------------
// Event rings
//---------------------------------------

/// @brief Construct a trb_event_ring object.
///
/// @param max_entries How many entries are going to be in this event ring? The valid range is 6-4096, inclusive.
///
/// @param interrupter Which interrupter number should this event ring service? Maximum 1023.
///
/// @param parent The parent controller for this ring.
trb_event_ring::trb_event_ring(uint16_t max_entries, uint16_t interrupter, controller *parent) :
  consumer_cycle_state_bit(true),
  number_of_entries(max_entries)
{
  KL_TRC_ENTRY;

  ASSERT((max_entries >= 16) && (max_entries <= 4096));
  ASSERT(interrupter < 1024);

  // Create an event ring space.
  ring_ptr_virt = new basic_trb[max_entries];
  for (int i = 0; i < max_entries; i++)
  {
    ring_ptr_virt[i].populate();
  }

  start_of_ring_phys = mem_get_phys_addr(ring_ptr_virt);

  // Create an event ring segment table.
  erst = new event_ring_seg_table_entry;
  erst_phys = mem_get_phys_addr(erst);

  erst->segment_size = max_entries;
  erst->segment_phys_base_addr = start_of_ring_phys;

  // Fill in the event ring registers.
  our_interrupt_reg = &parent->interrupters[interrupter];

  our_interrupt_reg->table_size = 1;
  our_interrupt_reg->erst_dequeue_ptr_phys = reinterpret_cast<uint64_t>(start_of_ring_phys);
  our_interrupt_reg->interrupt_management.enable = 1;
  our_interrupt_reg->erst_base_addr_phys = reinterpret_cast<uint64_t>(erst_phys);

  dequeue_ptr = reinterpret_cast<template_trb *>(ring_ptr_virt);

  KL_TRC_EXIT;
}

trb_event_ring::~trb_event_ring()
{
  delete[] ring_ptr_virt;
  delete erst;
}

/// @brief Retrieve the next TRB from the queue.
///
/// If there are no more TRBs to read after this one, update the xHCI's ERDP.
///
/// @param[out] dequeue_trb The next TRB is written to this pointer.
///
/// @return True if there was a TRB to dequeue. False otherwise.
bool trb_event_ring::dequeue_trb(basic_trb *dequeue_trb)
{
  bool result = false;
  template_trb *old_dequeue_ptr;

  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Dequeue ptr: ", ((basic_trb *)dequeue_ptr)->reserved_1, ", ", ((basic_trb *)dequeue_ptr)->reserved_2, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Dequeue ptr + 1: ", ((basic_trb *)(dequeue_ptr + 1))->reserved_1, ", ", ((basic_trb *)(dequeue_ptr + 1))->reserved_2, "\n");

  if (!dequeue_trb)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Dequeue ptr is null\n");
  }
  else if ((dequeue_ptr->cycle == 1) == consumer_cycle_state_bit)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Dequeue this TRB\n");
    copy_trb(dequeue_trb, reinterpret_cast<basic_trb *>(dequeue_ptr));

    old_dequeue_ptr = dequeue_ptr;
    dequeue_ptr++;

    if ((dequeue_ptr - reinterpret_cast<template_trb *>(ring_ptr_virt)) >= number_of_entries)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Resetting pointer to start\n");
      dequeue_ptr = reinterpret_cast<template_trb *>(ring_ptr_virt);
      consumer_cycle_state_bit = !consumer_cycle_state_bit;
    }

    // That is to say, if the next TRB can't be dequeued yet.
    if ((dequeue_ptr->cycle == 1) != consumer_cycle_state_bit)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No more TRBs, updating dequeue ptr\n");
      our_interrupt_reg->erst_dequeue_ptr_phys = reinterpret_cast<uint64_t>(mem_get_phys_addr(old_dequeue_ptr));
    }

    result = true;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}
