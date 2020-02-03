/// @file
/// @brief Implements a USB Human Interface Device.

// Known defects:
// - We allocate a buffer for decoded reports in advance, which could lead to some multithreading issues if we get two
//   reports simultaneously.

#define ENABLE_TRACING

#include "usb_hid_device.h"
#include "hid_input_reports.h"
#include "hid_usages.h"

#include "usb_hid_mouse.h"
#include "usb_hid_keyboard.h"

#include <klib/klib.h>

namespace usb
{

/// @brief Standard constructor.
///
/// @param core The controller-specific core of this HID device.
///
/// @param interface_num The interface number of this specific HID device.
hid_device::hid_device(std::shared_ptr<generic_core> core, uint16_t interface_num) :
  generic_device{core, interface_num, "USB Human Interface Device"},
  interrupt_in_endpoint_num{0xFF},
  report_packet_size{0}
{
  bool success = true;
  KL_TRC_ENTRY;

  // Do a few quick checks that endpoints and so on are as-expected. We expect at least an IN direction endpoint.
  if ((active_interface.endpoints[0].attributes.transfer_type != 3) || // check is an interrupt endpoint...
      ((active_interface.endpoints[0].endpoint_address & 0x80) == 0)) // ... in the IN direction.
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Endpoint assumptions wrong.\n");
    success = false;
  }

  interrupt_in_endpoint_num = active_interface.endpoints[0].endpoint_address & 0x0F;
  report_packet_size = active_interface.endpoints[0].max_packet_size;

  if (success)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Endpoint assumptions OK, read descriptor\n");
    success = read_hid_descriptor(interface_hid_descriptor);
  }

  if (success)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "HID descriptor found\n");

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Num class descriptors: ", interface_hid_descriptor.num_hid_class_descriptors, "\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Report type code: ", interface_hid_descriptor.report_descriptor_type, "\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Report length: ", interface_hid_descriptor.report_descriptor_length, "\n");

    if (interface_hid_descriptor.num_hid_class_descriptors != 1)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "More than one HID class descriptor is unsupported.\n");
      success = false;
    }
  }

  if (success)
  {
    device_request_type rt;

    KL_TRC_TRACE(TRC_LVL::FLOW, "Got sensible HID descriptor, try report descriptor.\n");

    rt.direction = 1;
    rt.recipient = 1;

    raw_class_descriptor = std::unique_ptr<uint8_t[]>(new uint8_t[interface_hid_descriptor.report_descriptor_length]);
    success = core->get_descriptor(interface_hid_descriptor.report_descriptor_type,
                                   0,
                                   device_interface_num,
                                   interface_hid_descriptor.report_descriptor_length,
                                   raw_class_descriptor.get(),
                                   rt.raw);
#ifdef ENABLE_TRACING
    if (success)
    {
      KL_TRC_TRACE(TRC_LVL::EXTRA, "Raw descriptor: \n");
      for (uint16_t i = 0; i < interface_hid_descriptor.report_descriptor_length; i++)
      {
        KL_TRC_TRACE(TRC_LVL::EXTRA, raw_class_descriptor[i], "\n");
      }
    }
#endif
  }

  if (success)
  {
    success = usb::hid::parse_descriptor(raw_class_descriptor,
                                         interface_hid_descriptor.report_descriptor_length,
                                         report_descriptor);

#ifdef ENABLE_TRACING
    if (success)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Decoded the following input fields: \n");

      for (const usb::hid::report_field_description &f : report_descriptor.input_fields)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, f.num_bits, " bits @ ",
                      f.byte_offset, ":", f.bit_offset,
                      ". Usage: ", f.usage, "\n");
      }
    }
#endif
  }

  if (success)
  {
    device_request_type rt;

    KL_TRC_TRACE(TRC_LVL::FLOW, "Got descriptor, set protocol to 'Report'\n");

    // Set the device protocol to report, but also enable any specialisations that we know about.
    create_specialisation();

    rt.raw = 0x21;
    success = device_core->device_request(rt, HID_DEVICE_REQUESTS::SET_PROTOCOL, 1, device_interface_num, 0, nullptr);
  }

  if (!success)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to start HID device\n");
    set_device_status(DEV_STATUS::FAILED);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Ready, stopped\n");
    set_device_status(DEV_STATUS::STOPPED);
  }

  KL_TRC_EXIT;
}

bool hid_device::start()
{
  bool result{true};
  bool success{false};

  KL_TRC_ENTRY;

  if (get_device_status() == DEV_STATUS::STOPPED)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Set report mode, schedule a transfer (", report_packet_size, " bytes) and begin!\n");

    decode_buffer = std::unique_ptr<int64_t[]>(new int64_t[report_descriptor.input_fields.size()]);
    current_transfer = std::make_shared<normal_transfer>(this,
                                                         std::unique_ptr<uint8_t[]>(new uint8_t[report_packet_size]),
                                                         report_packet_size);
    success = device_core->queue_transfer(interrupt_in_endpoint_num,
                                          true,
                                          current_transfer);
  }

  if (success)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Started device OK\n");
    set_device_status(DEV_STATUS::OK);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to start HID device.\n");
    set_device_status(DEV_STATUS::FAILED);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool hid_device::stop()
{
  bool result{true};

  KL_TRC_ENTRY;

  INCOMPLETE_CODE("HID device stop()");

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool hid_device::reset()
{
  bool result{true};

  KL_TRC_ENTRY;

  INCOMPLETE_CODE("HID device reset()");

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Retrieve and store the HID descriptor associated with the interface being used for this device.
///
/// @param[out] storage The HID descriptor, if found, will be copied here.
///
/// @return True if the HID descriptor was successfully copied, false otherwise.
bool hid_device::read_hid_descriptor(hid_descriptor &storage)
{
  bool result = false;
  klib_list_item<descriptor_header *> *item = nullptr;
  descriptor_header *hdr = nullptr;

  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Looking at descriptors for config idx ", device_core->active_configuration,
                               ", interface ", device_interface_num, "\n");

  item = device_core->configurations[device_core->active_configuration].
                    interfaces[device_interface_num].other_descriptors.head;

  while (item != nullptr)
  {
    hdr = item->item;
    KL_TRC_TRACE(TRC_LVL::FLOW, "Descriptor type: ", hdr->descriptor_type, "\n");

    if (hdr->descriptor_type == DESCRIPTOR_TYPES::HID)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "HID Descriptor found\n");
      memcpy(&storage, hdr, sizeof(storage));
      result = true;
      break;
    }

    item = item->next;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void hid_device::transfer_completed(normal_transfer *complete_transfer)
{
  bool success;
  uint16_t num_fields;

  KL_TRC_ENTRY;

  if (complete_transfer == current_transfer.get())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Event: ",
                                complete_transfer->transfer_buffer[0], ".",
                                complete_transfer->transfer_buffer[1], ".",
                                complete_transfer->transfer_buffer[2], ".",
                                complete_transfer->transfer_buffer[3], "\n");

    num_fields = report_descriptor.input_fields.size();
    ASSERT(decode_buffer);

    success = hid::parse_report(report_descriptor,
                                complete_transfer->transfer_buffer.get(),
                                complete_transfer->buffer_size,
                                decode_buffer.get(),
                                num_fields);

    // If we can natively understand this report, then deal with it.
    if (success && child_specialisation)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Deal with specialised report\n");
      child_specialisation->process_report(report_descriptor, decode_buffer.get(), num_fields);
    }

    if (!success)
    {
      KL_TRC_TRACE(TRC_LVL::IMPORTANT, "Failed to decode HID report\n");
    }

    // Queue up a new transfer.
    current_transfer = std::make_shared<normal_transfer>(this,
                                                         std::unique_ptr<uint8_t[]>(new uint8_t[report_packet_size]),
                                                         report_packet_size);
    success = device_core->queue_transfer(interrupt_in_endpoint_num,
                                          true,
                                          current_transfer);


    if (!success)
    {
      INCOMPLETE_CODE("No failure handler");
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown transfer event.");
  }


  KL_TRC_EXIT;
}

/// @brief If the device has revealed a specialisation that we know about, create a sub-device to handle those reports.
///
void hid_device::create_specialisation()
{
  KL_TRC_ENTRY;

  if (report_descriptor.root_collection.child_collections.size() > 0)
  {
    switch (report_descriptor.root_collection.child_collections[0].usage)
    {
    case hid::USAGE::MOUSE:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found mouse specialisation!\n");
      child_specialisation = std::make_unique<hid::mouse>();
      break;

    case hid::USAGE::KEYBOARD:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found keyboard specialisation!\n");
      child_specialisation = std::make_unique<hid::keyboard>();
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "No known specialisation\n");
    }
  }

  KL_TRC_EXIT;
}

} // Namespace usb
