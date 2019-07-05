/// @file
/// @brief Structures and constants associated with making control requests to USB devices.

#pragma once

#include <stdint.h>

namespace usb
{
#pragma pack (push, 1)
  // There is little value in documenting these namespace in Doxygen, so skip them. Simply refer to the USB spec for
  // further details.
  /// @cond
  namespace DEV_REQ_TYPE
  {
    const uint8_t STANDARD = 0;
    const uint8_t CLASS = 1;
    const uint8_t VENDOR = 2;
  };

  /// @brief USB Device request recipients
  ///
  /// Other values are reserved.
  namespace DEV_REQ_RECIP
  {
    const uint8_t DEVICE = 0;
    const uint8_t INTERFACE = 1;
    const uint8_t ENDPOINT = 2;
    const uint8_t OTHER = 3;
    const uint8_t VENDOR = 31;
  };

  /// @brief Standard USB device request codes.
  ///
  /// This includes codes in the USB 3.2 spec up to September 2017.
  namespace DEV_REQUEST
  {
    const uint8_t GET_STATUS = 0;
    const uint8_t CLEAR_FEATURE = 1;
    const uint8_t SET_FEATURE = 3;
    const uint8_t SET_ADDRESS = 5;
    const uint8_t GET_DESCRIPTOR = 6;
    const uint8_t SET_DESCRIPTOR = 7;
    const uint8_t GET_CONFIGURATION = 8;
    const uint8_t SET_CONFIGURATION = 9;
    const uint8_t GET_INTERFACE = 10;
    const uint8_t SET_INTERFACE = 11;
    const uint8_t SYNCH_FRAME = 12;
    const uint8_t SET_ENCRYPTION = 13;
    const uint8_t GET_ENCRYPTION = 14;
    const uint8_t SET_HANDSHAKE = 15;
    const uint8_t GET_HANDSHAKE = 16;
    const uint8_t SET_CONNECTION = 17;
    const uint8_t SET_SECURITY_DATA = 18;
    const uint8_t GET_SECURITY_DATA = 19;
    const uint8_t SET_WUSB_DATA = 20;
    const uint8_t LOOPBACK_DATA_WRITE = 21;
    const uint8_t LOOPBACK_DATA_READ = 22;
    const uint8_t SET_INTERFACE_DS = 23;
    const uint8_t SET_SEL = 48;
    const uint8_t SET_ISOCH_DELAY = 49;
  };

  /// @brief USB Device standard descriptor types
  ///
  /// For use with get_descriptor()
  namespace DESCRIPTOR_TYPES
  {
    const uint8_t DEVICE = 1;
    const uint8_t CONFIGURATION = 2;
    const uint8_t STRING = 3;
    const uint8_t INTERFACE = 4;
    const uint8_t ENDPOINT = 5;
    const uint8_t INTERFACE_POWER = 8;
    const uint8_t OTG = 9;
    const uint8_t DEBUG = 10;
    const uint8_t INTERFACE_ASSOCIATION = 11;
    const uint8_t BOS = 15;
    const uint8_t DEVICE_CAPABILITY = 16;
    const uint8_t SUPERSPEED_USB_ENDPOINT_COMPANION = 48;
    const uint8_t SUPERSPEEDPLUS_ISOCHRONOUS_ENDPOINT_COMPANION = 49;

    // Values not specified in the core specification, but usefully contained here:
    const uint8_t HID = 0x21; ///< HID. Given by the HID spec.
  }
  /// @endcond

  /// @brief Helper structure for filling in the Request Type byte of USB device requests.
  ///
  /// See the spec for further details.
  struct device_request_type
  {
    /// @cond
    union
    {
      struct
      {
        uint8_t recipient : 4;
        uint8_t type : 3;
        uint8_t direction : 1;
      };
      uint8_t raw{0};
    };
    /// @endcond
  };
  static_assert(sizeof(device_request_type) == 1, "Device request type size wrong.");

  //---------------------------------
  // Standard descriptor structures.
  //---------------------------------

  /// @brief Standard header for USB descriptors
  ///
  /// Can be used when the descriptor type is not known.
  struct descriptor_header
  {
    uint8_t length; ///< Length of the descriptor, in bytes.

    /// The type of the descriptor. May be one of DESCRIPTOR_TYPES, or may be a device-specific value.
    ///
    uint8_t descriptor_type;
  };
  static_assert(sizeof(descriptor_header) == 2, "Descriptor header size wrong.");

  /// @brief Standard USB device descriptor.
  ///
  /// Contains basic details about the device. This is a standard USB structure - see the USB spec for further details.
  struct device_descriptor
  {
    /// @cond
    uint8_t length;
    uint8_t descriptor_type;
    uint16_t usb_ver_bcd;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    uint8_t max_packet_size_encoded;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_ver_bcd;
    uint8_t manufacturer_string_idx;
    uint8_t product_string_idx;
    uint8_t serial_number_idx;
    uint8_t num_configurations;
  /// @endcond
  };
  static_assert(sizeof(device_descriptor) == 18, "Device descriptor size wrong.");

  /// @brief Standard USB device configuration descriptor.
  ///
  /// USB devices may support multiple configurations, with different capabilities. Each configuration descriptor is
  /// followed by one or more interface descriptors.
  struct config_descriptor
  {
    uint8_t length; ///< Total length of this descriptor.
    uint8_t descriptor_type; ///< Will be set to DESCRIPTOR_TYPES::CONFIGURATION.
    uint16_t total_length; ///< Length of this descriptor and all following interface and endpoint descriptors.
    uint8_t num_interfaces; ///< The number of interfaces supported in this configuration.
    uint8_t config_index_number; ///< The index for this configuration to be sent over the wire to the device.
    uint8_t config_string_idx; ///< The string index for the string describing this configuration.
    uint8_t attributes; ///< Attributes, as given in table 9-22 of the USB3.2 spec.
    uint8_t max_power; ///< Coded maximum power drawn by this device.
  };
  static_assert(sizeof(config_descriptor) == 9, "Config descriptor size wrong.");

  /// @brief Standard USB device interface descriptor.
  ///
  /// Each interface descriptor is followed by num_endpoints endpoint_descriptor structures.
  struct interface_descriptor
  {
    uint8_t length; ///< Length of this descriptor.
    uint8_t descriptor_type; ///< Will be set to DESCRIPTOR_TYPES::INTERFACE
    uint8_t interface_number; ///< Index number of this interface.
    uint8_t interface_alternate_num; ///< Index number of the alternate mode of this interface.
    uint8_t num_endpoints; ///< How many endpoints does this interface use. If 0, only uses the control endpoint.
    uint8_t interface_class; ///< Class code for this interface.
    uint8_t interface_subclass; ///< Subclass code for this interface.
    uint8_t interface_protocol; ///< Protocol number used for this interface.
    uint8_t interface_string_idx; ///< Index of the string describing this interface.
  };
  static_assert(sizeof(interface_descriptor) == 9, "Interface descriptor size wrong.");

  /// @brief Standard USB endpoint descriptor.
  ///
  /// The details of this structure can be found in the USB specification.
  struct endpoint_descriptor
  {
    /// @cond
    uint8_t length; ///< Length of this descriptor.
    uint8_t descriptor_type; ///< Will be set to DESCRIPTOR_TYPES::ENDPOINT.
    uint8_t endpoint_address; ///< Coded endpoint address.
    union
    {
      uint8_t raw; ///< Attributes in raw format.
      struct
      {
        uint8_t transfer_type : 2; ///< 0: control, 1: isochronous, 2: bulk, 3: interrupt.
        uint8_t synch_type : 2; ///< Meaning depends on transfer type.
        uint8_t usage : 2; ///< Meaning depends on transfer type.
        uint8_t reserved : 2; ///< Reserved.
      };
    } attributes; ///< Coded description of the endpoint type.
    uint16_t max_packet_size; ///< Maximum packet size for this endpoint, in bytes.
    uint8_t service_interval; ///< How often the endpoint requires servicing, in 125us increments.
    /// @endcond
  };
  static_assert(sizeof(endpoint_descriptor) == 7, "Endpoint descriptor size wrong.");

#pragma pack (pop)
};