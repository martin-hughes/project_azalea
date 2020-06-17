/// @file
/// @brief PCI Generic Device bus-mastering control functions.

// Known defects:
// - No locking around PCI register accesses - what if another thread attempts concurrent modification?

//#define ENABLE_TRACING

#include "kernel_all.h"
#include "pci_generic_device.h"
#include "../arch/x64/processor/processor-x64.h"

/// @brief Enable Bus Mastering for this PCI device.
///
/// It is unspecified what happens if bus mastering is not supported.
///
/// @return Always true - we always attempt to enable bus mastering.
bool pci_generic_device::bm_enable()
{
  bool result{true};
  pci_reg_1 cmd_reg;

  KL_TRC_ENTRY;

  cmd_reg.raw = pci_read_raw_reg(_address, PCI_REGS::STATUS_AND_COMMAND);
  cmd_reg.bus_master_enable = 1;
  pci_write_raw_reg(_address, PCI_REGS::STATUS_AND_COMMAND, cmd_reg.raw);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Disable Bus Mastering for this PCI device.
///
/// It is unspecified what happens if bus mastering is not supported, but there *should* be no effect.
///
/// @return Always true - we always attempt to disable bus mastering.
bool pci_generic_device::bm_disable()
{
  bool result{true};
  pci_reg_1 cmd_reg;

  KL_TRC_ENTRY;

  cmd_reg.raw = pci_read_raw_reg(_address, PCI_REGS::STATUS_AND_COMMAND);
  cmd_reg.bus_master_enable = 0;
  pci_write_raw_reg(_address, PCI_REGS::STATUS_AND_COMMAND, cmd_reg.raw);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Is Bus Mastering enabled for this PCI device?
///
/// This function attempts to determine whether bus mastering is enabled for a given PCI device by reading the command
/// register. It is possible that the bus mastering bit will be set even if the device doesn't support bus mastering,
/// in which case this function will return an incorrect result.
///
/// @return True if the bus mastering enable bit is set in the device's command register, false otherwise.
bool pci_generic_device::bm_enabled()
{
  bool result{true};
  pci_reg_1 cmd_reg;

  KL_TRC_ENTRY;

  cmd_reg.raw = pci_read_raw_reg(_address, PCI_REGS::STATUS_AND_COMMAND);
  result = (cmd_reg.bus_master_enable == 1);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
