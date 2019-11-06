/// @file
/// @brief Declare types to handle TRB rings in xHCI controllers.

#pragma once

#include "devices/usb/controllers/usb_xhci_register_types.h"
#include "devices/usb/controllers/usb_xhci_trb_types.h"

#include "klib/synch/kernel_locks.h"

#include <queue>

namespace usb { namespace xhci {

  void copy_trb(template_trb *dest, template_trb *src);
  class controller;
  struct xhci_command_data;

  /// @brief A generic xHCI ring.
  ///
  /// Manages a single ring, as used by the xHCI in several contexts.
  class trb_generic_ring
  {
  protected:
    // A "generic" TRB ring doesn't exist - each type has its own idiosyncrasies.
    trb_generic_ring(uint16_t max_entries);

  public:
    virtual ~trb_generic_ring();

    void *get_phys_base_address();

    virtual bool queue_trb(template_trb *new_trb, uint32_t &segment_position);
    virtual bool queue_trb(template_trb *new_trb);

  protected:
    template_trb *trb_ring_base; ///< The virtual-space address of the first TRB in the ring.
    void *trb_allocation; ///< The base address of the allocation used to store the ring.
    void *trb_ring_base_phys; ///< The physical address of the first TRB in the ring.
    uint16_t segment_size; ///< How many TRBs fit in this ring?

    template_trb *enqueue_ptr; ///< The virtual-space enqueue pointer for the ring.
    bool cycle_bit; ///< The current state of the cycle bit relevant to this ring type.

    /// @brief Is this a valid TRB for this ring type?
    ///
    /// This function must be overridden, since there is no 'generic' ring type and hence no TRBs are valid for it.
    ///
    /// @param new_trb The TRB under test
    ///
    /// @return True if the TRB is valid for this ring, false otherwise.
    virtual bool is_valid_trb(template_trb &new_trb) = 0;
  };

  /// @brief An xHCI Transfer Ring.
  ///
  class trb_transfer_ring : public trb_generic_ring
  {
  public:
    trb_transfer_ring(uint16_t max_entries);

    virtual bool queue_ctrl_transfer(setup_stage_transfer_trb *setup_trb,
                                     data_stage_transfer_trb *data_trbs,
                                     status_stage_transfer_trb *status_trb,
                                     uint32_t num_data_trbs,
                                     uint64_t &trb_phys_addr);

    virtual bool queue_regular_transfer(void *buffer, uint32_t transfer_length, uint64_t &trb_phys_addr);

    virtual bool is_valid_trb(template_trb &new_trb) override;
  };

  /// @brief An xHCI command ring.
  ///
  class trb_command_ring : public trb_generic_ring
  {
  public:
    trb_command_ring(uint16_t max_entries);

    virtual bool queue_command(xhci_command_data *new_command);
    virtual xhci_command_data *retrieve_command(command_completion_event_trb &trb);

    virtual bool is_valid_trb(template_trb &new_trb) override;

  protected:
    /// This array allows us to, using the index of the TRB in the ring, look up what command generated that TRB. It
    /// assumes that commands are processed in order.
    std::unique_ptr<std::queue<xhci_command_data *>[]> command_queues;
    kernel_spinlock queue_lock; ///< Lock controlling access to command_queues.

    uint32_t convert_phys_to_position(uint64_t phys_trb_addr);
  };

  /// @brief An xHCI event ring.
  ///
  class trb_event_ring
  {
  public:
    trb_event_ring(uint16_t max_entries, uint16_t interrupter, controller *parent);
    virtual ~trb_event_ring();

    virtual bool dequeue_trb(template_trb *dequeue_trb);
    virtual void set_handler_not_busy();

  protected:
    /// @brief A single Event Ring Segment Table Entry.
    ///
    /// See xHCI spec section 6.5 for details.
    struct event_ring_seg_table_entry
    {
      void *segment_phys_base_addr; ///< The physical address of this segment of the ring.
      uint16_t segment_size; ///< The number of contiguous TRBs in this segment.
      uint16_t reserved_1; ///< Reserved.
      uint32_t reserved_2; ///< Reserved.
    };
    static_assert(sizeof(event_ring_seg_table_entry) == 16, "Sizeof ERST entry wrong.");

    bool consumer_cycle_state_bit; ///< Our copy of the CCS bit.
    uint16_t number_of_entries; ///< How many entries are there in the event ring.

    template_trb *dequeue_ptr; ///< Our copy of the dequeue pointer.
    template_trb *ring_ptr_virt; ///< Virtual address of the beginning of the event ring.
    void *start_of_ring_phys; ///< Physical address of the beginning of the event ring.

    event_ring_seg_table_entry *erst; ///< Virtual address of the Event Ring Segment Table for this ring.
    void *erst_phys; ///< Physical address of the ERST for this ring.

    interrupter_regs *our_interrupt_reg; ///< Pointer to the interrupter for this ring.
  };

}; };