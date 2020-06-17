/// @file
/// @brief Implements xHCI rings.

// Known deficiencies
// - we make no attempt to check rings don't cross 64kB boundaries.
// - There is no attempt at thread safety!
// - In several places in this file and elsewhere we assume that only interrupter 0 is in use...

//#define ENABLE_TRACING

#include "kernel_all.h"
#include "usb_xhci_controller.h"
#include "timing.h"

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
  /// @param src The source of the copy.
  void copy_trb(template_trb *dest, template_trb *src)
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
  trb_allocation = kmalloc((max_entries + 1) * sizeof(template_trb));
  ASSERT(trb_allocation != nullptr);

  addr_num = reinterpret_cast<uint64_t>(trb_allocation);
  offset = addr_num % 16;
  if (offset != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Realigning allocation.");
    addr_num = addr_num + 16 - offset;
  }

  trb_ring_base = reinterpret_cast<template_trb *>(addr_num);
  enqueue_ptr =trb_ring_base;

  for (int i = 0; i < max_entries - 1; i++)
  {
    trb_ring_base[i].populate();
  }

  // Calculate the ring's physical address and store it for later.
  trb_ring_base_phys = mem_get_phys_addr(trb_ring_base);

  final_trb = reinterpret_cast<link_trb *>(&trb_ring_base[max_entries - 1]);
  final_trb->populate(trb_ring_base_phys, 0, 0, 0, 0, 0);

  segment_size = max_entries;

  // The ring cycle state bit starts as true.
  cycle_bit = true;

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
/// @param[in] new_trb Pointer the TRB to queue.
///
/// @param[out] segment_position The position of the TRB within the ring segment, in number of TRBs from the beginning
///                              of the segment. This may be useful if the caller wants to correlate later events to
///                              this TRB.
///
/// @return True if the TRB could be queued. False otherwise.
bool trb_generic_ring::queue_trb(template_trb *new_trb, uint32_t &segment_position)
{
  bool result = false;

  KL_TRC_ENTRY;

  if (new_trb == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "TRB nullptr\n");
    result = false;
  }
  else if (!is_valid_trb(*new_trb))
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
    // Set the cycle bit as required.
    new_trb->cycle = (cycle_bit ? 1 : 0);

    // Copy the TRB.
    copy_trb(enqueue_ptr, new_trb);
    segment_position = enqueue_ptr - trb_ring_base;

    // Advance the enqueue pointer. For a simplification, we know that our ring only contains one link TRB that points
    // back to the start.
    enqueue_ptr++;
    if (enqueue_ptr->trb_type == TRB_TYPES::LINK)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Returning to beginning of ring\n");
      enqueue_ptr = trb_ring_base;
      cycle_bit = !cycle_bit;
    }

    result = true;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Queue a TRB at the enqueue pointer.
///
/// @param[in] new_trb Pointer the TRB to queue.
///
/// @return True if the TRB could be queued. False otherwise.
bool trb_generic_ring::queue_trb(template_trb *new_trb)
{
  uint32_t dummy = 0;
  return queue_trb(new_trb, dummy);
}

//---------------------------------------
// Transfer rings
//---------------------------------------

/// @brief Create a new TRB transfer ring.
///
/// @param max_entries The number of TRBs to fit in the ring.
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

/// @brief Queue a control endpoint transfer
///
/// This is only a valid operation on control endpoints, but this function will appear to complete correctly on any
/// endpoint
///
/// @param[in] setup_trb Pointer to the setup stage trb to send.
///
/// @param[in] data_trbs Optional pointer to one or more data stage TRBs to queue. Set to nullptr if no data stage TRBs
///                      are needed. If nullptr, num_data_trbs must be zero. If not, num_data_trbs must not be zero.
///
/// @param[in] status_trb Pointer to the status stage TRB.
///
/// @param[in] num_data_trbs The number of data stage TRBs to be queued. If 0, data_trbs must be nullptr. If not,
///                          data_trbs must not be nullptr.
///
/// @param[out] trb_phys_addr Physical address of the status stage TRB when it is on the transfer ring. This is
///                           provided so that the caller can correlate transfer requests to results.
///
/// @return True if the transfer was queued successfully, false otherwise.
bool trb_transfer_ring::queue_ctrl_transfer(setup_stage_transfer_trb *setup_trb,
                                            data_stage_transfer_trb *data_trbs,
                                            status_stage_transfer_trb *status_trb,
                                            uint32_t num_data_trbs,
                                            uint64_t &trb_phys_addr)
{
  bool result = true;
  uint32_t posn;

  KL_TRC_ENTRY;

  if (((data_trbs != nullptr) && (num_data_trbs == 0)) ||
      ((data_trbs == nullptr) && (num_data_trbs != 0)) ||
      (setup_trb == nullptr) ||
      (status_trb == nullptr) ||
      (num_data_trbs > (this->segment_size - 3)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parameters\n");
    result = false;
  }
  else
  {
    setup_trb->interrupt_on_complete = false;
    queue_trb(reinterpret_cast<template_trb *>(setup_trb));

    for (uint64_t i = 0; i < num_data_trbs; i++)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Queueing TRB ", i, "\n");
      data_trbs[i].interrupt_on_complete = false;
      queue_trb(reinterpret_cast<template_trb *>(data_trbs + i));
    }

    result = queue_trb(reinterpret_cast<template_trb *>(status_trb), posn);

    trb_phys_addr = reinterpret_cast<uint64_t>(this->trb_ring_base_phys) + (sizeof(template_trb) * posn);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Queue a normal transfer on this ring.
///
/// Note that the ring doesn't care whether it's an input or output transfer.
///
/// If this is an IN endpoint, and less data is transferred than expected, then an interrupt is still generated and the
/// transfer marked as complete. It is the caller's responsibility to deal with this.
///
/// @param buffer The buffer either containing data to send, or the buffer to receive data in to - depending on the
///               endpoint direction.
///
/// @param transfer_length The number of bytes to transfer.
///
/// @param[out] trb_phys_addr Physical address of the status stage TRB when it is on the transfer ring. This is
///                           provided so that the caller can correlate transfer requests to results.
///
/// @return True if the transfer was queued successfully, false otherwise.
bool trb_transfer_ring::queue_regular_transfer(void *buffer, uint32_t transfer_length, uint64_t &trb_phys_addr)
{
  bool result = true;
  normal_transfer_trb trb;
  uint32_t posn;

  KL_TRC_ENTRY;

  if (transfer_length >= (1 << 17))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Transfer length too long\n");
    result = false;
  }
  else
  {
    trb.populate(reinterpret_cast<uint64_t>(mem_get_phys_addr(buffer)),
                 transfer_length,
                 1,
                 0,
                 false,
                 false,
                 true,
                 false,
                 false,
                 true,
                 false,
                 false);

    result = queue_trb(reinterpret_cast<template_trb *>(&trb), posn);

    trb_phys_addr = reinterpret_cast<uint64_t>(this->trb_ring_base_phys) + (sizeof(template_trb) * posn);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

//---------------------------------------
// Command rings
//---------------------------------------

/// @brief Construct a Command Ring.
///
/// @param max_entries The number of entries to include in the ring.
trb_command_ring::trb_command_ring(uint16_t max_entries) : trb_generic_ring(max_entries)
{
  KL_TRC_ENTRY;

  ipc_raw_spinlock_init(this->queue_lock);
  command_queues = std::unique_ptr<std::queue<xhci_command_data *>[]>(new std::queue<xhci_command_data*>[max_entries]);

  KL_TRC_EXIT;
}

/// @brief Queues a new command on this command ring.
///
/// This function differs from queue_trb in that it also adds the command object to a list of commands awaiting a
/// response from the TRB, so that when a response is received it can be correlated with the command that calls it.
///
/// If the caller wishes to keep track of commands manually, queue_trb is still a reasonable alternative.
///
/// @param new_command The command to queue.
///
/// @return True if the command could be successfully queued, false otherwise.
bool trb_command_ring::queue_command(xhci_command_data *new_command)
{
  bool result;
  uint32_t queue_posn;

  KL_TRC_ENTRY;

  ipc_raw_spinlock_lock(this->queue_lock);

  result = queue_trb(&new_command->generated_trb, queue_posn);
  if (result == true)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Queued TRB, add to command queue\n");
    command_queues[queue_posn].push(new_command);
  }

  ipc_raw_spinlock_unlock(this->queue_lock);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}

/// @brief Retrieves the command that triggered the provided command completion event.
///
/// This function retrieves the first queued command that was associated with the TRB at the address provided by the
/// command completion event TRB given as a parameter. It is *assumed* that this will be the command that triggered the
/// command completion event that has itself triggered the call to this function. If not, the result is uncertain.
///
/// @param trb A command completion event TRB that is correlated with a command queued earlier using queue_command().
///
/// @return Assuming that a command that correlates to this TRB can be found, that command. nullptr otherwise.
xhci_command_data *trb_command_ring::retrieve_command(command_completion_event_trb &trb)
{
  xhci_command_data *result = nullptr;
  uint32_t position_in_segment;

  ipc_raw_spinlock_lock(this->queue_lock);

  position_in_segment = convert_phys_to_position(trb.command_trb_phys_addr);
  if ((position_in_segment != ~0) && (!command_queues[position_in_segment].empty()))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found command data\n");
    result = command_queues[position_in_segment].front();
    command_queues[position_in_segment].pop();
  }

  ipc_raw_spinlock_unlock(this->queue_lock);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
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

/// @brief Converts the physical address of a TRB into a position within the command ring segment.
///
/// @param phys_trb_addr The physical address of a TRB within the segment.
///
/// @return The position within the segment of that TRB, given in terms of the number of TRBs since the start. If the
///         physical address is invalid, returns ~0.
uint32_t trb_command_ring::convert_phys_to_position(uint64_t phys_trb_addr)
{
  uint32_t posn;
  uint64_t base_addr_int = reinterpret_cast<uint64_t>(this->trb_ring_base_phys);

  KL_TRC_ENTRY;

  posn = (phys_trb_addr - base_addr_int) / sizeof(template_trb);
  if (posn > this->segment_size)
  {
    posn = ~0;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", posn, "\n");
  KL_TRC_EXIT;

  return posn;
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
  ring_ptr_virt = new template_trb[max_entries];
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

  dequeue_ptr = ring_ptr_virt;

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
bool trb_event_ring::dequeue_trb(template_trb *dequeue_trb)
{
  bool result = false;

  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Dequeue ptr: ", dequeue_ptr->reserved_1, ", ", dequeue_ptr->reserved_2, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Dequeue ptr + 1: ", (dequeue_ptr + 1)->reserved_1, ", ",
                              (dequeue_ptr + 1)->reserved_2, "\n");

  if (!dequeue_trb)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Dequeue ptr is null\n");
  }
  else if ((dequeue_ptr->cycle == 1) == consumer_cycle_state_bit)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Dequeue this TRB\n");
    copy_trb(dequeue_trb, dequeue_ptr);

    dequeue_ptr++;

    if ((dequeue_ptr - ring_ptr_virt) >= number_of_entries)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Resetting pointer to start\n");
      dequeue_ptr = ring_ptr_virt;
      consumer_cycle_state_bit = !consumer_cycle_state_bit;
    }

    // That is to say, if the next TRB can't be dequeued yet.
    if ((dequeue_ptr->cycle == 1) != consumer_cycle_state_bit)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No more TRBs, updating dequeue ptr\n");
      set_handler_not_busy();
    }

    result = true;
  }

  if (!result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No TRB, set handler not busy.\n");
    set_handler_not_busy();
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}

/// @brief Update the dequeue pointer to clear the busy bit and allow additional TRBs to come to us.
void trb_event_ring::set_handler_not_busy()
{
  KL_TRC_ENTRY;

  uint64_t temp_dequeue = reinterpret_cast<uint64_t>(mem_get_phys_addr(dequeue_ptr)) | 8;
  our_interrupt_reg->erst_dequeue_ptr_phys = temp_dequeue;

  KL_TRC_EXIT;
}
