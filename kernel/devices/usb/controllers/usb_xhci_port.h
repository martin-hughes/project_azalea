/// @file
/// @brief Header file for xHCI root ports.

#pragma once

#include "devices/usb/controllers/usb_xhci_register_types.h"
#include "devices/usb/controllers/usb_xhci_capabilities.h"
#include "devices/usb/usb_gen_device.h"

namespace usb { namespace xhci
{
  class controller;
  class device_core;

  // These names just come from the xHCI spec.
  /// @cond
  enum class PORT_STS
  {
    INVALID,

    NOT_CONFIGURED,
    POWERED_OFF,
    DISCONNECTED,
    DISABLED,
    RESETTING,
    ENABLED,
    TRANSMIT,
    TRANSMIT_R,
    SUSPENDED,
    RESUMING,
    SEND_EOR,
    RESTART_S,
    RESTART_E,
    POLLING,
  };
  /// @endcond

  /// @brief Manages a root port
  ///
  /// Users of the port are advised to use this class rather than trying to manage the port directly.
  class root_port
  {
  public:
    root_port();
    root_port(controller *our_parent,
              uint16_t port_id,
              port_regs *port_regs_base,
              supported_protocols_cap *our_cap_ptr);
    virtual ~root_port();

    void port_status_change_event();
    uint8_t get_required_slot_type();
    port_regs *get_raw_reg();
    uint16_t get_default_max_packet_size();

    void handle_child_device_addressed();

    bool is_valid_port();

  protected:
    PORT_STS calculate_current_status();

    controller *parent; ///< The controller that owns this port.
    bool valid_port; ///< Is the port valid and operating?
    bool usb3; ///< Is this a USB-3 port?
    port_regs *our_port_reg; ///< Pointer to the parent controller's port register for this port.
    PORT_STS most_recent_status; ///< Our most recently calculated status - which may be out of date.
    uint8_t port_id; ///< The Port ID given to us by our parent.
    uint8_t slot_type; ///< The slot type (an opaque ID number given in the capabilities)
    supported_protocols_cap *our_capability; ///< Pointer to the extended capability covering this port.
    protocol_speed_id *speed_id_table; ///< Not supported.
    std::shared_ptr<device_core> xhci_core; ///< The child device's core object spawned by this port.
    std::shared_ptr<usb::generic_device> child_device; ///< If a device is instantiated, it is referenced here.
  };

}; };