/// @file
/// @brief Declares a standard USB-HID.

#pragma once

#include "devices/usb/usb_gen_device.h"
#include "hid_input_reports.h"
#include "usb_hid_specialisation.h"

namespace usb
{
#pragma pack (push, 1)
  /// @brief USB descriptor for HID interfaces.
  ///
  /// Should come immediately after the interface descriptor with a HID class value.
  struct hid_descriptor
  {
    uint8_t length; ///< Length of this descriptor - should be 9 or greater.
    uint8_t descriptor_type; ///< Type of this descriptor - must be DESCRIPTOR_TYPES::HID.
    uint16_t hid_spec_version_bcd; ///< BCD value of the spec this descriptor conforms to. We understand 1.11
    uint8_t country_code; ///< Target country of this hardware.
    uint8_t num_hid_class_descriptors; ///< Number of class descriptors that can be loaded.
    uint8_t report_descriptor_type; ///< The value of "descriptor_type" for the Report descriptor. (Usually 0x22)
    uint16_t report_descriptor_length; ///< Length of the report descriptor.
  };
  static_assert(sizeof(hid_descriptor) == 9, "Sizeof HID descriptor wrong.");

#pragma pack (pop)

  /// @brief Request values to send to the device as part of a request.
  ///
  namespace HID_DEVICE_REQUESTS
  {
    const uint8_t GET_REPORT = 1; ///< Request a report via the control pipe.
    const uint8_t GET_IDLE = 2; ///< Get the idle rate for a particular input report.
    const uint8_t GET_PROTOCOL = 3; ///< Get the protocol (boot, report) currently in use.
    const uint8_t SET_REPORT = 9; ///< Send a report to the device (perhaps to set output fields).
    const uint8_t SET_IDLE = 10; ///< Set the idle rate for a particular input report.
    const uint8_t SET_PROTOCOL = 11; ///< Set the protocol in use to either Boot or Report.
  }

  /// @brief A HID class driver.
  ///
  class hid_device : public generic_device
  {
  public:
    hid_device(std::shared_ptr<generic_core> core, uint16_t interface_num);
    virtual ~hid_device() = default;

    virtual void transfer_completed(normal_transfer *complete_transfer) override;

    // Overrides of IDevice
    virtual bool start() override;
    virtual bool stop() override;
    virtual bool reset() override;

  protected:
    hid_descriptor interface_hid_descriptor; ///< Store an easy-to-access copy of the HID descriptor for this interface
    std::unique_ptr<uint8_t[]> raw_class_descriptor; ///< Stores the HID class descriptor.
    usb::hid::decoded_descriptor report_descriptor; ///< The report descriptor for this device, in decoded format.
    uint8_t interrupt_in_endpoint_num; ///< The number of the interrupt IN endpoint serving this device.
    uint16_t report_packet_size; ///< Maximum size of inbound reports.
    std::shared_ptr<normal_transfer> current_transfer; ///< The transfer object currently being used for data transfers.

    /// If the HID device is of a known type, store the specialisation here.
    ///
    std::unique_ptr<hid::hid_specialisation> child_specialisation;

    std::unique_ptr<int64_t[]> decode_buffer; ///< Storage for decoding an input report in to.

    bool read_hid_descriptor(hid_descriptor &storage);
    void create_specialisation();
  };
}
