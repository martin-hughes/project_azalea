/// @file
/// @brief Implement support for some of the USB xHCI extended capabilities.

//#define ENABLE_TRACING

#include "usb_xhci_controller.h"
#include "usb_xhci_capabilities.h"

using namespace usb::xhci;

/// @brief Iterate over all extended capabilities and integrate the useful ones.
///
/// @return true if this was successful. False if some error prevented a successful iteration.
bool controller::examine_extended_caps()
{
  extended_cap_hdr *this_cap;
  uint64_t cap_offset;
  bool failed = false;
  bool found_sup_proto_cap = false;
  bool result;
  KL_TRC_ENTRY;

  this_cap = extended_caps;

  while (this_cap != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found capability: ", this_cap->cap_id, "\n");

    switch(this_cap->cap_id)
    {
    case EXT_CAPS::SUPPORTED_PROTOCOL:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Supported protocol capability\n");
      // There must be at least one of these, so having seen one it seems as though the list is valid.
      found_sup_proto_cap = true;

      failed = !examine_proto_support_cap(reinterpret_cast<supported_protocols_cap *>(this_cap)) || failed;

      break;

    case EXT_CAPS::LEGACY_SUP:
    case EXT_CAPS::EXT_POWER_MGMT:
    case EXT_CAPS::IO_VIRTUALIZATION:
    case EXT_CAPS::MSG_INTERRUPT:
    case EXT_CAPS::LOCAL_MEM:
    case EXT_CAPS::USB_DEBUG:
    case EXT_CAPS::EXT_MSG_INTERRUPT:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not currently supported\n");
    }

    if (this_cap->next_cap_ptr != 0)
    {
      cap_offset = this_cap->next_cap_ptr << 2;
      this_cap = reinterpret_cast<extended_cap_hdr *>(reinterpret_cast<uint64_t>(this_cap) + cap_offset);
    }
    else
    {
      this_cap = nullptr;
    }
  }

  result = found_sup_proto_cap & !failed;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Examine a Supported Protocols capability structure.
///
/// From this structure, populate any relevant port control objects with the data found here.
///
/// @param cap The capability structure to examine.
///
/// @return True if the structure was successfully examined. False otherwise.
bool controller::examine_proto_support_cap(supported_protocols_cap *cap)
{
  bool result = true;
  KL_TRC_ENTRY;

  if (cap->cap_id != EXT_CAPS::SUPPORTED_PROTOCOL)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong capability ID\n");
    result = false;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "USB version: ", cap->revision_major, " / ", cap->revision_minor, "\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "USB string: ", cap->name_string, "\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "Ports: ", cap->compatible_port_offset, " / ", cap->compatible_port_count, "\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "PSIDs: ", cap->protocol_speed_id_count, "\n");

    if (cap->protocol_speed_id_count != 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Don't know what to do with PSIDs\n");
      result = false;
    }


    for (int i = cap->compatible_port_offset; i < (cap->compatible_port_offset + cap->compatible_port_count); i++)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Initialize port ", i, "\n");
      root_ports[i] = root_port(this, i, port_control_regs, cap);
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}