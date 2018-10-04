/// @file
/// @brief MSI control functions for generic PCI devices.

//#define ENABLE_TRACING

#include "devices/pci/generic_device/pci_generic_device.h"
#include "processor/x64/processor-x64.h"

#include "klib/klib.h"

/// @brief Configure MSI, but don't start using it yet.
///
/// This is still only a basic implementation. All interrupts are sent to the BSP.
///
/// @param[in] interrupts_requested The number of interrupts requested for use by the device. Must be a power of two
///                                 less than or equal to 32.
///
/// @param[out] interrupts_granted. The number of interrupts granted to this device.
///
/// @return True if MSI was successfully configured. False otherwise. If false, do not attempt to enable MSI.
bool pci_generic_device::msi_configure(uint8_t interrupts_requested, uint8_t &interrupts_granted)
{
  pci_msi_cap_header msi_hdr;
  uint8_t max_supported;
  uint8_t start_vector;
  uint8_t compacted_num_vectors;
  uint8_t msg_data_reg_offset;
  uint64_t msg_address;

  KL_TRC_ENTRY;

  // Quick sanity checking.
  if (!caps.msi.supported)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "MSI not supported\n");
    interrupts_granted = 0;
  }
  else if ((interrupts_requested > 32) || (interrupts_requested == 0))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Wrong number of interrupts requested\n");
    interrupts_granted = 0;
  }
  else if (round_to_power_two(interrupts_requested) != interrupts_requested)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Only power-of-two requests supported\n");
    interrupts_granted = 0;
  }
  else
  {
    // What is the maximum number of interrupts we could *actually* get?
    msi_hdr.raw = pci_read_raw_reg(_address, caps.msi.offset / 4);
    ASSERT (msi_hdr.cap_label == PCI_CAPABILITY_IDS::MSI);
    max_supported = 1 << msi_hdr.multiple_msg_capable;

    if (interrupts_requested > max_supported)
    {
      interrupts_granted = max_supported;
    }
    else
    {
      interrupts_granted = interrupts_requested;
    }

    KL_TRC_TRACE(TRC_LVL::FLOW, "Maximum supported: ", max_supported, ", granted: ", interrupts_granted, "\n");

    if (!proc_request_interrupt_block(interrupts_granted, start_vector))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to gain interrupt block\n");
      interrupts_granted = 0;
    }
    else
    {
      compacted_num_vectors = which_power_of_two(interrupts_granted);
      msi_hdr.multiple_msg_enable = compacted_num_vectors;
      msi_hdr.msi_enable = 0;

      msg_data_reg_offset = msi_hdr.cap_64_bit_addr ? 12 : 8;
      KL_TRC_TRACE(TRC_LVL::FLOW, "Offset to message data field: ", msg_data_reg_offset, "\n");

      // Always send interrupts to the first processor, for now.
      msg_address = proc_x64_generate_msi_address(0);
      KL_TRC_TRACE(TRC_LVL::FLOW, "MSI address: ", msg_address, "\n");

      // Write the capability register
      pci_write_raw_reg(_address, caps.msi.offset / 4, msi_hdr.raw);

      // Write the start vector in to the message data register. This simply uses edge-triggered mode. I think that's
      // OK.
      pci_write_raw_reg(_address, (caps.msi.offset + msg_data_reg_offset) / 4, start_vector);
      ASSERT(pci_read_raw_reg(_address, (caps.msi.offset + msg_data_reg_offset) / 4) == start_vector);

      // Write the message address register
      pci_write_raw_reg(_address, (caps.msi.offset + 4) / 4, static_cast<uint32_t>(msg_address));

      KL_TRC_TRACE(TRC_LVL::FLOW, "Start vector: ", start_vector, "\n");
      _base_interrupt_vector = start_vector;
      _num_allocated_vectors = interrupts_granted;
    }
  }

  KL_TRC_EXIT;
  return (interrupts_granted != 0);
}

/// @brief Begin sending message signalled interrupts.
///
/// Assuming it has been configured, enable MSI and start sending using it.
///
/// The caller is responsible for serializing access to all MSI functions, otherwise device configuration may be
/// corrupted and the device could fail.
///
/// @return True if MSI is now enabled, false otherwise.
bool pci_generic_device::msi_enable()
{
  pci_msi_cap_header msi_hdr;
  bool result;

  KL_TRC_ENTRY;

  // Quick sanity checking.
  if (!caps.msi.supported)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "MSI not supported\n");
    result = false;
  }
  else
  {
    for (int i = 0; i < _num_allocated_vectors; i++)
    {
      proc_register_interrupt_handler(_base_interrupt_vector + i, this);
    }

    msi_hdr.raw = pci_read_raw_reg(_address, caps.msi.offset / 4);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Before: ", msi_hdr.raw);

    msi_hdr.msi_enable = 1;
    pci_write_raw_reg(_address, caps.msi.offset / 4, msi_hdr.raw);

    KL_TRC_TRACE(TRC_LVL::FLOW, " / After: ", pci_read_raw_reg(_address, caps.msi.offset / 4), "\n");

    result = true;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Cease sending message signalled interrupts.
///
/// The caller is responsible for serializing access to all MSI functions, otherwise device configuration may be
/// corrupted and the device could fail.
///
/// @return True if MSI is now disabled, false if it is still in use or we couldn't change the MSI config for some
///         reason.
bool pci_generic_device::msi_disable()
{
  pci_msi_cap_header msi_hdr;
  bool result;

  KL_TRC_ENTRY;

  // Quick sanity checking.
  if (!caps.msi.supported)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "MSI not supported\n");
    result = false;
  }
  else
  {
    msi_hdr.raw = pci_read_raw_reg(_address, caps.msi.offset / 4);
    msi_hdr.msi_enable = 0;
    pci_write_raw_reg(_address, caps.msi.offset / 4, msi_hdr.raw);

    for (int i = 0; i < _num_allocated_vectors; i++)
    {
      proc_unregister_interrupt_handler(_base_interrupt_vector + i, this);
    }

    result = true;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
