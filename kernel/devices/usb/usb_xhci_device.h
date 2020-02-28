/// @file
/// @brief Declares the "core" part of a USB device for devices connected to an xHCI controller.

#pragma once

#include <map>

#include <stdint.h>

#include "devices/usb/usb_gen_device.h"
#include "devices/usb/controllers/usb_xhci_controller.h"

namespace usb { namespace xhci {

  /// @brief An xHCI device core part.
  ///
  class device_core : public usb::generic_core
  {
  protected:
    device_core(controller *parent, uint8_t port, root_port *parent_port);
  public:
    static std::shared_ptr<device_core> create(controller *parent, uint8_t port, root_port *parent_port);
    virtual ~device_core() override;

    // Overrides of Generic USB Core functions
    virtual bool device_request(device_request_type request_type,
                                uint8_t request,
                                uint16_t value,
                                uint16_t index,
                                uint16_t length,
                                void *buffer) override;

    virtual uint16_t get_max_packet_size() override;
    virtual bool set_max_packet_size(uint16_t new_packet_size) override;
    virtual bool configure_device(uint8_t config_num) override;
    virtual void configuration_set() override;
    virtual bool queue_transfer(uint8_t endpoint_num,
                                bool is_inwards,
                                std::shared_ptr<normal_transfer> transfer_item) override;

    // Override of work::message_receiver
    virtual void handle_message(std::unique_ptr<msg::root_msg> &message) override;

    // xHCI-specific functions.
    virtual void handle_slot_enabled(uint8_t slot_id, device_context *new_output_context);
    virtual void handle_addressed();
    virtual uint8_t get_port_num();
    virtual void handle_transfer_event(transfer_event_trb &trb);
    virtual void endpoints_configured();

  protected:
    controller *parent; ///< Pointer to the parent controller device.
    uint8_t port_num; ///< The number of the port this device is connected to.
    root_port *parent_port; ///< Pointer to the port structure this device is connected to.

    /// An input context for providing to the xHCI. Maintaining one seems to be easier than continually allocating and
    /// deallocating one. Note that this is for input only, to see the current state of the device, use dev_context.
    std::unique_ptr<input_context> dev_input_context;

    /// Transfer ring for this device's default control endpoint.
    ///
    std::unique_ptr<trb_transfer_ring> def_ctrl_endpoint_transfer_ring;
    uint8_t slot_id; ///< The slot ID for this device.
    uint16_t current_max_packet_size; ///< The maximum packet size supported on the default control interface.

    /// TRB transfer rings for each possible endpoint. If nullptr, then no ring is associated with the corresponding
    /// endpoint. The first index represents the endpoint number. For the second index, 0 = output, 1 = input.
    std::unique_ptr<trb_transfer_ring> transfer_rings[16][2];

    /// Map the physical addresses of transfer TRBs to work response items, so that the response item can be marked
    /// complete when the relevant transfer event TRB is received.
    std::map<uint64_t, std::shared_ptr<normal_transfer>> current_transfers;

    device_context *dev_context; ///< Pointer to the device context, as seen by the xHCI. Do not directly modify.

    kernel_spinlock current_transfers_lock; ///< Lock to protect current_transfers.
  };

}; };