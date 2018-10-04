#pragma once

#include "devices/usb/controllers/usb_gen_controller.h"
#include "devices/usb/controllers/usb_xhci_register_types.h"
#include "devices/usb/controllers/usb_xhci_trb_types.h"
#include "devices/usb/controllers/usb_xhci_trb_ring.h"
#include "klib/klib.h"
#include <memory>

namespace usb { namespace xhci
{
  /// @brief An implementation of the USB xHCI specification
  ///
  class controller : public usb_gen_controller
  {
    friend trb_event_ring;

  public:
    controller(pci_address address);
    virtual ~controller();

    virtual const kl_string device_name() override;
    virtual DEV_STATUS get_device_status() override;

    virtual bool handle_translated_interrupt_fast(unsigned char interrupt_offset,
                                                  unsigned char raw_interrupt_num) override;

  protected:
    // Basic driver support.
    const kl_string _dev_name = kl_string("USB XHCI controller");
    DEV_STATUS _dev_status;

    // XHCI structures
    // ---------------

    // Initialization helpers.
    void initialize_registers(pci_address address);
    bool initial_hardware_check();
    bool prepare_control_structures();
    uint64_t generate_scratchpad_array(uint16_t num_scratchpads);

    // Member variables.
    volatile caps_regs *capability_regs;
    volatile oper_regs *operational_regs;
    std::unique_ptr<device_context *[]> device_ctxt_base_addr_array;
    trb_command_ring command_ring;
    uint64_t runtime_regs_virt_addr;
    volatile uint32_t *doorbell_regs;
    interrupter_regs *interrupters;

    // I'm going to manage the memory for this one manually, since the individual objects can't be managed via
    // new/delete. This is a list of the physical pages we've allocated to store scratchpad buffers.
    uint64_t *scratchpad_virt_array_ptr;
    uint64_t *scratchpad_phys_page_ptr_array;
    uint16_t num_scratchpad_page_ptrs;

    // For now, only include one event ring.
    std::unique_ptr<trb_event_ring> event_ring;
  };
}};