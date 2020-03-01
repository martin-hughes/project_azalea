/// @file
/// @brief USB xHCI extended capabilities structures.

#pragma once

#include <stdint.h>

namespace usb { namespace xhci {

#pragma pack(push, 1)
  /// @brief Contains constants used in the xHCI 'extended capabilities' registers.
  ///
  namespace EXT_CAPS
  {
    const uint8_t RESERVED = 0; ///< Reserved.
    const uint8_t LEGACY_SUP = 1; ///< Legacy Support capability.
    const uint8_t SUPPORTED_PROTOCOL = 2; ///< Supported protocols capability.
    const uint8_t EXT_POWER_MGMT = 3; ///< Extended power management capabilty.
    const uint8_t IO_VIRTUALIZATION = 4; ///< I/O virtualization capability.
    const uint8_t MSG_INTERRUPT = 5; ///< Non-PCI Message interrupt capability.
    const uint8_t LOCAL_MEM = 6; ///< Local Memory capability.
    const uint8_t USB_DEBUG = 10; ///< USB-provided debug port capability.
    const uint8_t EXT_MSG_INTERRUPT = 17; ///< Non-PCI Extended message interrupt capability.
  }

  /// @brief Header for all xHCI extended capabilities.
  ///
  struct extended_cap_hdr
  {
    uint32_t cap_id : 8; ///< The type of this capability. One of EXT_CAPS.
    uint32_t next_cap_ptr : 8; ///< Offset of the next extended capability.
    uint32_t revision_minor : 8; ///< Minor version number of the capability structure.
    uint32_t revision_major : 8; ///< Major version number of the capability structure.
  };
  static_assert(sizeof(extended_cap_hdr) == 4, "Caps header size wrong");

  /// @brief Structure of the xHCI Supported Protocols Capability.
  ///
  struct supported_protocols_cap
  {
    uint32_t cap_id : 8; ///< The type of this capability. Set to EXT_CAPS::SUPPORTED_PROTOCOL
    uint32_t next_cap_ptr : 8; ///< Offset of the next extended capability.
    uint32_t revision_minor : 8; ///< Minor version number of the capability structure.
    uint32_t revision_major: 8; ///< Major version number of the capability structure.
    uint32_t name_string; ///< Constant that, taken with revision_minor/major, defines the supported USB version.

    uint32_t compatible_port_offset : 8; ///< The first port of the root hub supporting this protocol.
    uint32_t compatible_port_count : 8; ///< The number of consecutive root hub ports supporting this protocol.

    uint32_t protocol_defined : 12; ///< Protocol specific definitions.
    uint32_t protocol_speed_id_count : 4; ///< The number of protocol speed IDs following this structure (UNSUPPORTED)

    uint32_t protocol_slot_type : 5; ///< The value of slot type to use when allocating a slot for this port.
    uint32_t reserved : 27; ///< Reserved.
  };
  static_assert(sizeof(unsigned char) == sizeof(uint8_t), "Incorrect character size");
  static_assert(sizeof(supported_protocols_cap) == 16, "Incorrect size of supported_protocols_cap");

  /// @brief Protocol speed ID DWORD used in conjunction with supported_protocols_cap.
  ///
  /// While defined, currently unsupported.
  struct protocol_speed_id
  {
    uint32_t psi_val : 4; ///< The ID value of this PSID.
    uint32_t psi_exponent : 2; ///< 0 = bps, 1 = kbps, 2 = Mbps, 3 = Gbps.
    uint32_t psi_type : 2; ///< See xHCI spec.
    uint32_t psi_full_duplex : 1; ///< See xHCI spec.
    uint32_t reserved : 5; ///< Reserved.
    uint32_t link_protocol : 2; ///< See xHCI spec.
    uint32_t psi_mantissa : 16; ///< See xHCI spec.
  };
  static_assert(sizeof(protocol_speed_id) == 4, "Wrong size of protocol_speed_id");

#pragma pack(pop)
}; };
