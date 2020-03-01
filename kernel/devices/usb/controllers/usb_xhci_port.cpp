/// @file
/// @brief Implements control of the root hub ports on an xHCI controller.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "devices/usb/controllers/usb_xhci_port.h"
#include "devices/usb/controllers/usb_xhci_controller.h"
#include "devices/usb/usb_xhci_device.h"
#include "devices/usb/usb.h"

using namespace usb::xhci;

/// @brief Create a default control object for an xHCI root port.
///
/// This port is not valid - it doesn't correspond to a port on the xHCI yet.
root_port::root_port() : parent{nullptr},
                         valid_port{false},
                         usb3{false},
                         our_port_reg{nullptr},
                         most_recent_status{PORT_STS::NOT_CONFIGURED},
                         port_id{0},
                         slot_type{0},
                         our_capability{nullptr},
                         speed_id_table{nullptr}
{

}

/// @brief Initialize the control object for an xHCI root port.
///
/// @param our_parent Pointer to the parent controller. Must not be nullptr.
///
/// @param port_id The port number for the root port. Valid values are 0 - MAX_PORTS.
///
/// @param port_regs_base Pointer to the parent controller's port registers.
///
/// @param our_cap_ptr Pointer to the Extended Capabilities Support Protocol Capability that covers this port.
root_port::root_port(controller *our_parent,
                     uint16_t port_id,
                     port_regs *port_regs_base,
                     supported_protocols_cap *our_cap_ptr) :
  root_port{}
{
  KL_TRC_ENTRY;

  parent = our_parent;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Port ", port_id, ":\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Major / minor: ", our_cap_ptr->revision_major, ", ", our_cap_ptr->revision_minor, "\n");

  ASSERT(our_cap_ptr != nullptr);
  ASSERT(port_regs_base != nullptr);
  ASSERT(our_parent != nullptr);

  if ((our_cap_ptr->name_string == 0x20425355) &&
      ((our_cap_ptr->revision_major == 2) || (our_cap_ptr->revision_major == 3)) &&
      (port_id > 0) &&
      (port_id <= 255))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Valid port details\n");
    valid_port = true;

    if (our_cap_ptr->revision_major == 3)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "USB 3 port\n");
      usb3 = true;
    }

    our_port_reg = &port_regs_base[port_id - 1];
    this->port_id = port_id;
    this->slot_type = our_cap_ptr->protocol_slot_type;
    this->our_capability = our_cap_ptr;
    this->speed_id_table = reinterpret_cast<protocol_speed_id *>(reinterpret_cast<unsigned char *>(our_capability) +
                                                                 sizeof(supported_protocols_cap));
  }

  KL_TRC_EXIT;
}

root_port::~root_port()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

/// @brief Handle a Port Status Change Event aimed at this port.
///
void root_port::port_status_change_event()
{
  PORT_STS new_status;
  std::shared_ptr<usb::generic_device> new_device;

  KL_TRC_ENTRY;

  ASSERT(valid_port);

  // What has happened?
  new_status = calculate_current_status();

  if (new_status != most_recent_status)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Status change. Advance towards ENABLED.\n");
    switch(new_status)
    {
    case PORT_STS::NOT_CONFIGURED:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not configured. Nothing to do.\n");
      break;

    case PORT_STS::POWERED_OFF:
      INCOMPLETE_CODE("Powered off port.");
      break;

    case PORT_STS::DISCONNECTED:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Disconnected, wait for connection\n");
      break;

    case PORT_STS::DISABLED:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Disabled, maybe attempt polling.\n");
      if ((!usb3) && (our_port_reg->status_ctrl.port_link_status == 7))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Begin port polling.\n");
        our_port_reg->status_ctrl.port_reset = 1;
      }
      break;

    case PORT_STS::POLLING:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Polling state, should advance automatically.\n");
      break;

    case PORT_STS::ENABLED:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Now enabled - init device\n");
      xhci_core = device_core::create(parent, port_id, this);
      break;

    case PORT_STS::RESETTING:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Resetting, should advance automatically.\n");
      break;

    default:
      INCOMPLETE_CODE("Unknown state of USB port");
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No observed change. Ignore.\n");
  }

  most_recent_status = new_status;

  KL_TRC_EXIT;
}

/// @brief Calculate the current status of this port from the information we can see.
///
/// @return The current port status.
PORT_STS root_port::calculate_current_status()
{
  PORT_STS result = PORT_STS::INVALID;
  KL_TRC_ENTRY;

  if (our_port_reg->status_ctrl.port_power == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Powered down\n");
    result = PORT_STS::POWERED_OFF;
  }
  else if (our_port_reg->status_ctrl.current_connect_status == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Disconnected\n");
    result = PORT_STS::DISCONNECTED;
  }
  else if (our_port_reg->status_ctrl.port_enabled == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Port disabled\n");
    result = PORT_STS::DISABLED;
  }
  else if (our_port_reg->status_ctrl.port_reset == 1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Port resetting\n");
    result = PORT_STS::RESETTING;
  }
  else
  {
    switch(our_port_reg->status_ctrl.port_link_status)
    {
    case 7:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Polling\n");
      result = PORT_STS::POLLING;
      break;

    case 6:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Inactive (suspended?)\n");
      result = PORT_STS::SUSPENDED;
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Enabled\n");
      result = PORT_STS::ENABLED;
    }
  }

  KL_TRC_EXIT;

  return result;
}

/// @brief Returns the 'protocol slot type' associated with this port.
///
/// @return The 'protocol slot type' associated with this port by the xHCI extended capabilities.
uint8_t root_port::get_required_slot_type()
{
  return this->slot_type;
}

/// @brief Returns a raw pointer to the port register structure.
///
/// @return A raw pointer to the port register associated with this port. Do not delete this pointer!
port_regs *root_port::get_raw_reg()
{
  return our_port_reg;
}

/// @brief Return the default value of the maximum packet size for this port.
///
/// For LS, FS, HS and SS ports this value is 8, 8, 64 and 512 respectively. If it isn't possible to determine the port
/// type yet, assume 8.
///
/// @return The default maximum packet size for this port, as described above.
uint16_t root_port::get_default_max_packet_size()
{
  uint16_t max_size = 8;
  uint8_t speed_idx;
  protocol_speed_id *speed_id_ptr = nullptr;

  KL_TRC_ENTRY;

  if (our_capability->protocol_speed_id_count == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Use standard speed values\n");
    switch (our_port_reg->status_ctrl.port_speed)
    {
    case 1:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Full-speed\n");
      max_size = 8;
      break;

    case 2:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Low-speed\n");
      max_size = 8;
      break;

    case 3:
      KL_TRC_TRACE(TRC_LVL::FLOW, "High-speed\n");
      max_size = 64;
      break;

    case 4:
    case 5:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Super-speed\n");
      max_size = 512;
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown speed\n");
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Use specified speed values\n");

    speed_idx = our_port_reg->status_ctrl.port_speed;
    for (uint16_t i = 1; i <= our_capability->protocol_speed_id_count; i++)
    {
      if (speed_id_table[i - 1].psi_val == speed_idx)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found valid speed index\n");
        speed_id_ptr = &speed_id_table[i - 1];
        break;
      }
    }

    if (speed_id_ptr != nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Looking at protocol speed ID ptr\n");

      // The port should be USB 3 in this case. Does this mean the max packet size is 512?
      INCOMPLETE_CODE("Don't support SSIC yet");
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid protocol speed ID\n");
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", max_size, "\n");
  KL_TRC_EXIT;

  return max_size;
}

/// @brief Handles the child device become addressed.
///
/// At this point we can give it to the USB device factory to load a suitable driver for it.
void root_port::handle_child_device_addressed()
{
  KL_TRC_ENTRY;

  usb::main_factory::create_device(this->xhci_core);

  KL_TRC_EXIT;
}

/// @brief Is this a legitimate, real, operating USB port?
///
/// @return True if so, false otherwise.
bool root_port::is_valid_port()
{
  return valid_port;
}
