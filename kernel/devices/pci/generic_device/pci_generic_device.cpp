/// @file
/// @brief Implements a generic and simple PCI device.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "pci_generic_device.h"

using namespace PCI_CAPABILITY_IDS;

pci_generic_device::pci_generic_device(pci_address address) :
  _address(address),
  _base_interrupt_vector(0),
  _num_allocated_vectors(0)
{
  pci_reg_0 dev_reg0;
  pci_reg_2 dev_reg2;

  KL_TRC_ENTRY;

  dev_reg0.raw = pci_read_raw_reg(address, PCI_REGS::DEV_AND_VENDOR_ID);
  dev_reg2.raw = pci_read_raw_reg(address, PCI_REGS::CC_SC_PROG_IF_AND_REV_ID);

  KL_TRC_TRACE(TRC_LVL::FLOW, "New PCI Device:\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Bus: ", address.bus, ", Slot: ", address.device, ", Func: ", address.function, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Vendor ID: ", dev_reg0.vendor_id, ", Device ID: ", dev_reg0.device_id, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Class: ", dev_reg2.class_code, ", Subclass: ", dev_reg2.subclass, "\n");

  zero_caps_list();
  scan_caps();

  KL_TRC_EXIT;
}

pci_generic_device::~pci_generic_device()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

const kl_string pci_generic_device::device_name()
{
  return kl_string("Generic PCI device");
}

DEV_STATUS pci_generic_device::get_device_status()
{
  return DEV_STATUS::FAILED;
}

/// @brief Initialize the list of capabilities to empty.
///
void pci_generic_device::zero_caps_list()
{
  KL_TRC_ENTRY;

  caps.pci_power_mgmt = {false, 0, nullptr};
  caps.agp = {false, 0, nullptr};
  caps.vital_prod_data = {false, 0, nullptr};
  caps.slot_ident = {false, 0, nullptr};
  caps.msi = {false, 0, nullptr};
  caps.compact_pci_hotswap = {false, 0, nullptr};
  caps.pci_x = {false, 0, nullptr};
  caps.hypertransport = {false, 0, nullptr};
  caps.vendor_specific_cap = {false, 0, nullptr};
  caps.debug_port = {false, 0, nullptr};
  caps.compact_pci_crc = {false, 0, nullptr};
  caps.pci_hotplug = {false, 0, nullptr};
  caps.pci_bridge_vendor_id = {false, 0, nullptr};
  caps.agp_8x = {false, 0, nullptr};
  caps.secure_device = {false, 0, nullptr};
  caps.pci_express = {false, 0, nullptr};
  caps.msi_x = {false, 0, nullptr};

  KL_TRC_EXIT;
}

/// @brief If the device supports PCI extended capabilities, enumerate them.
///
void pci_generic_device::scan_caps()
{
  pci_reg_1 status_reg;
  uint16_t next_offset = 0;

  KL_TRC_ENTRY;

  status_reg.raw = pci_read_raw_reg(_address, PCI_REGS::STATUS_AND_COMMAND);
  if (status_reg.new_caps_list)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "PCI capabilities supported, begin scan\n");
    pci_reg_13 caps_ptr;

    caps_ptr.raw = pci_read_raw_reg(_address, PCI_REGS::CAP_PTR);
    next_offset = caps_ptr.caps_offset & ~3;
  }

  while (next_offset != 0)
  {
    //KL_TRC_TRACE(TRC_LVL::FLOW, "Looking at offset: ", next_offset, "\n");
    pci_cap_header cap_hdr;

    cap_hdr.raw = pci_read_raw_reg(_address, next_offset / 4);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Cap found: ", cap_hdr.cap_label, " @ ", next_offset, "\n");

#define SET_CAP_SUPPORTED(cap_label, cap_obj, name) \
    case (cap_label): \
      KL_TRC_TRACE(TRC_LVL::FLOW, (name), " found.\n"); \
      (cap_obj).supported = true; \
      (cap_obj).offset = next_offset; \
      break;

    switch (cap_hdr.cap_label)
    {
    SET_CAP_SUPPORTED(PCI_POWER_MGMT, caps.pci_power_mgmt, "PCI Power Management capability\n");
    SET_CAP_SUPPORTED(AGP, caps.agp, "AGP capability\n");
    SET_CAP_SUPPORTED(VITAL_PROD_DATA, caps.vital_prod_data, "Vital Prod Data capability\n");
    SET_CAP_SUPPORTED(SLOT_IDENT, caps.slot_ident, "Slot Ident capability\n");
    SET_CAP_SUPPORTED(MSI, caps.msi, "MSI capability\n");
    SET_CAP_SUPPORTED(COMPACT_PCI_HOTSWAP, caps.compact_pci_hotswap, "Compact PCI hotswap\n");
    SET_CAP_SUPPORTED(PCI_X, caps.pci_x, "PCI-X capability");
    SET_CAP_SUPPORTED(HYPERTRANSPORT, caps.hypertransport, "Hypertransport capability");
    SET_CAP_SUPPORTED(VENDOR_SPECIFIC_CAP, caps.vendor_specific_cap, "Vendor-specific capability");
    SET_CAP_SUPPORTED(DEBUG_PORT, caps.debug_port, "Debug port capability");
    SET_CAP_SUPPORTED(COMPACT_PCI_CRC, caps.compact_pci_crc, "Compact PCI CRC capability");
    SET_CAP_SUPPORTED(PCI_HOTPLUG, caps.pci_hotplug, "PCI Hotplug capability");
    SET_CAP_SUPPORTED(PCI_BRIDGE_VENDOR_ID, caps.pci_bridge_vendor_id, "PCI Bridge Vendor ID capability");
    SET_CAP_SUPPORTED(AGP_8X, caps.agp_8x, "AGP 8x capability");
    SET_CAP_SUPPORTED(SECURE_DEVICE, caps.secure_device, "Secure Device capability");
    SET_CAP_SUPPORTED(PCI_EXPRESS, caps.pci_express, "PCI Express capablity");
    SET_CAP_SUPPORTED(MSI_X, caps.msi_x, "MSI-X capability");

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown capability ID: ", cap_hdr.cap_label, "\n");
    }

    next_offset = cap_hdr.next_cap_offset & ~3;
  }

  KL_TRC_EXIT;
}

bool pci_generic_device::handle_interrupt_fast(unsigned char interrupt_number)
{
  //KL_TRC_ENTRY;

  return handle_translated_interrupt_fast(interrupt_number - _base_interrupt_vector, interrupt_number);

  //KL_TRC_EXIT;
}

void pci_generic_device::handle_interrupt_slow(unsigned char interrupt_number)
{
  KL_TRC_ENTRY;

  handle_translated_interrupt_slow(interrupt_number - _base_interrupt_vector, interrupt_number);

  KL_TRC_EXIT;
}