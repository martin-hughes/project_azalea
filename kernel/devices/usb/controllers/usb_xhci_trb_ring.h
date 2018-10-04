#pragma once

#include "devices/usb/controllers/usb_xhci_register_types.h"
#include "devices/usb/controllers/usb_xhci_trb_types.h"

namespace usb { namespace xhci {

  void copy_trb(basic_trb *dest, basic_trb *src);
  class controller;

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

    virtual bool queue_trb(basic_trb *new_trb);

  protected:
    basic_trb *trb_ring_base; ///< The virtual-space address of the first TRB in the ring.
    void *trb_allocation; ///< The base address of the allocation used to store the ring.
    void *trb_ring_base_phys; ///< The physical address of the first TRB in the ring.

    template_trb *enqueue_ptr; ///< The virtual-space enqueue pointer for the ring.

    virtual bool is_valid_trb(template_trb &new_trb) = 0;
  };

  /// @brief An xHCI Transfer Ring.
  ///
  class trb_transfer_ring : public trb_generic_ring
  {
  public:
    trb_transfer_ring(uint16_t max_entries);

    virtual bool is_valid_trb(template_trb &new_trb) override;
  };

  /// @brief An xHCI command ring.
  ///
  class trb_command_ring : public trb_generic_ring
  {
  public:
    trb_command_ring(uint16_t max_entries);

    virtual bool is_valid_trb(template_trb &new_trb) override;
  };

  /// @brief An xHCI event ring.
  ///
  class trb_event_ring
  {
  public:
    trb_event_ring(uint16_t max_entries, uint16_t interrupter, controller *parent);
    virtual ~trb_event_ring();

    virtual bool dequeue_trb(basic_trb *dequeue_trb);

  protected:
    struct event_ring_seg_table_entry
    {
      void *segment_phys_base_addr;
      uint16_t segment_size;
      uint16_t reserved_1;
      uint32_t reserved_2;
    };
    static_assert(sizeof(event_ring_seg_table_entry) == 16, "Sizeof ERST entry wrong.");

    bool consumer_cycle_state_bit;
    uint16_t number_of_entries;

    template_trb *dequeue_ptr;
    basic_trb *ring_ptr_virt;
    void *start_of_ring_phys;

    event_ring_seg_table_entry *erst;
    void *erst_phys;

    interrupter_regs *our_interrupt_reg;
  };

}; };