/// @file
/// @brief An interface that specialisations of USB HID devices can derive from.
///
/// Specialisations of a HID device are device types that the system might recognise, like mice, keyboards, joysticks,
/// etc. The parent HID device can simply give the reports to objects that inherit this interface and they will behave
/// in the way the user expects.

#pragma once

#include "hid_input_reports.h"
#include "devices/device_interface.h"

namespace usb { namespace hid {

/// @brief An interface that specialisations of USB HID devices can derive from.
///
/// Specialisations of a HID device are device types that the system might recognise, like mice, keyboards, joysticks,
/// etc. The parent HID device can simply give the reports to objects that inherit this interface and they will behave
/// in the way the user expects.
class hid_specialisation : public IDevice
{
public:
  /// @brief Default constructor
  ///
  /// @param name Human readable name for this specialisation.
  hid_specialisation(const kl_string name) : IDevice{name} { };
  virtual ~hid_specialisation() = default; ///< Default destructor.

  /// @brief Process a report that has been assigned to this specialisation.
  ///
  /// @param descriptor The descriptor relevant to this report.
  ///
  /// @param values An array containing the decoded values corresponding to the fields in descriptor.
  ///
  /// @param num_values The number of entries in values, which may be less than the total number of fields expected.
  virtual void process_report(decoded_descriptor &descriptor, int64_t *values, uint16_t num_values) = 0;
};

}; }; // Namespace usb::hid
