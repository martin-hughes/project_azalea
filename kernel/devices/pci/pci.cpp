/// @file
/// @brief Generic PCI code
///
/// Functions useful for all PCI drivers that don't really fit in a particular place.

//#define ENABLE_TRACING

#include <stdint.h>

#include "klib/klib.h"
#include "devices/pci/pci.h"
#include "devices/pci/pci_constants.h"
#include "devices/pci/pci_structures.h"
#include "devices/pci/pci_functions.h"
#include "devices/pci/pci_drivers.h"
#include "processor/processor.h"

#include "devices/device_monitor.h"
#include "devices/usb/usb.h"
#include "devices/block/ata/controller/ata_pci_controller.h"
#include "devices/virtio/virtio.h"

/// @brief Read a 32-bit register from the PCI configuration space.
///
/// This function returns the complete 32-bit register. The caller is then responsible for accessing the desired field
/// within the register.
///
/// If invalid parameters are given, this function will panic.
///
/// @param address The full PCI address to send the request to. The 'enable' field is ignored.
///
/// @return The value read from the given PCI address.
uint32_t pci_read_raw_reg (pci_address address)
{
  pci_address addr;
  uint32_t ret;

  KL_TRC_ENTRY;

  addr.raw = address.raw;
  addr.enable = 1;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Querying PCI address: ", addr.raw, "\n");

  proc_write_port (0xCF8, addr.raw, 32);
  ret = (uint32_t)proc_read_port (0xCFC, 32);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ret, "\n");
  KL_TRC_EXIT;

  return ret;
}

/// @brief Read a 32-bit register from the PCI configuration space.
///
/// This function returns the complete 32-bit register. The caller is then responsible for accessing the desired field
/// within the register.
///
/// If invalid parameters are given, this function will panic.
///
/// @param address The PCI address to query. Only the bus, slot and function fields are required.
///
/// @param reg PCI register number. Valid values are 0-63, inclusive. Each register is 4 bytes wide.
///
/// @return The value read from the given PCI address.
uint32_t pci_read_raw_reg (pci_address address, PCI_REGS reg)
{
  pci_address addr;
  addr.raw = address.raw;
  addr.register_num = static_cast<uint8_t>(reg);

  return pci_read_raw_reg(addr);
}

/// @brief Read a 32-bit register from the PCI configuration space.
///
/// This function returns the complete 32-bit register. The caller is then responsible for accessing the desired field
/// within the register.
///
/// If invalid parameters are given, this function will panic.
///
/// @param address The PCI address to query. Only the bus, slot and function fields are required.
///
/// @param reg PCI register number. Valid values are 0-63, inclusive. Each register is 4 bytes wide.
///
/// @return The value read from the given PCI address.
uint32_t pci_read_raw_reg (pci_address address, uint8_t reg)
{
  pci_address addr;
  addr.raw = address.raw;
  addr.register_num = reg;

  return pci_read_raw_reg(addr);
}

/// @brief Read a 32-bit register from the PCI configuration space.
///
/// This function returns the complete 32-bit register. The caller is then responsible for accessing the desired field
/// within the register.
///
/// If invalid parameters are given, this function will panic.
///
/// @param bus PCI bus number. Can be any uint8_t value.
///
/// @param slot PCI slot number. Valid values are 0-31, inclusive.
///
/// @param func PCI device function number. Valid values are 0-7, inclusive.
///
/// @param reg PCI register number. Valid values are 0-63, inclusive. Each register is 4 bytes wide.
///
/// @return The value read from the given PCI address.
uint32_t pci_read_raw_reg (uint8_t bus, uint8_t slot, uint8_t func, uint8_t reg)
{
  pci_address addr;

  ASSERT(slot < 32);
  ASSERT(func < 8);
  ASSERT(reg < 64);

  addr.raw = 0;
  addr.bus = bus;
  addr.device = slot;
  addr.function = func;
  addr.register_num = reg;
  addr.enable = 1;

  return pci_read_raw_reg(addr);
}

/// @brief Read a 32-bit register from the PCI configuration space.
///
/// This function returns the complete 32-bit register. The caller is then responsible for accessing the desired field
/// within the register.
///
/// If invalid parameters are given, this function will panic.
///
/// @param bus PCI bus number. Can be any uint8_t value.
///
/// @param slot PCI slot number. Valid values are 0-31, inclusive.
///
/// @param func PCI device function number. Valid values are 0-7, inclusive.
///
/// @param reg PCI register.
///
/// @return The value read from the given PCI address.
uint32_t pci_read_raw_reg (uint8_t bus, uint8_t slot, uint8_t func, PCI_REGS reg)
{
  return pci_read_raw_reg(bus, slot, func, static_cast<uint8_t>(reg));
}

/// @brief Write a 32-bit register to the PCI configuration space.
///
/// This function writes the complete 32-bit register each time.
///
/// If invalid parameters are given, this function will panic.
///
/// @param address The full PCI address to send the request to. The 'enable' field is ignored.
///
/// @param value The value to write into the register
void pci_write_raw_reg (pci_address address, uint32_t value)
{
  pci_address addr;

  KL_TRC_ENTRY;

  addr.raw = address.raw;
  addr.enable = 1;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Writing PCI address: ", addr.raw, "\n");

  proc_write_port (0xCF8, addr.raw, 32);
  proc_write_port (0xCFC, value, 32);

  KL_TRC_EXIT;
}

/// @brief Write a 32-bit register to the PCI configuration space.
///
/// This function writes the complete 32-bit register each time.
///
/// If invalid parameters are given, this function will panic.
///
/// @param address The PCI address to query. Only the bus, slot and function fields are required.
///
/// @param reg PCI register number. Valid values are 0-63, inclusive.
///
/// @param value The value to write into the register
void pci_write_raw_reg (pci_address address, PCI_REGS reg, uint32_t value)
{
  pci_address addr;
  addr.raw = address.raw;
  addr.register_num = static_cast<uint8_t>(reg);

  pci_write_raw_reg(addr, value);
}

/// @brief Write a 32-bit register to the PCI configuration space.
///
/// This function writes the complete 32-bit register each time.
///
/// If invalid parameters are given, this function will panic.
///
/// @param address The PCI address to query. Only the bus, slot and function fields are required.
///
/// @param reg PCI register number. Valid values are 0-63, inclusive.
///
/// @param value The value to write into the register
void pci_write_raw_reg (pci_address address, uint8_t reg, uint32_t value)
{
  pci_address addr;
  addr.raw = address.raw;
  addr.register_num = reg;

  pci_write_raw_reg(addr, value);
}

/// @brief Write a 32-bit register to the PCI configuration space.
///
/// This function writes the complete 32-bit register each time.
///
/// If invalid parameters are given, this function will panic.
///
/// @param bus PCI bus number. Can be any uint8_t value.
///
/// @param slot PCI slot number. Valid values are 0-31, inclusive.
///
/// @param func PCI device function number. Valid values are 0-7, inclusive.
///
/// @param reg PCI register number. Valid values are 0-63, inclusive. Each register is 4 bytes wide.
///
/// @param value The value to write into the register
void pci_write_raw_reg (uint8_t bus, uint8_t slot, uint8_t func, uint8_t reg, uint32_t value)
{
  pci_address addr;

  ASSERT(slot < 32);
  ASSERT(func < 8);
  ASSERT(reg < 64);

  addr.raw = 0;
  addr.bus = bus;
  addr.device = slot;
  addr.function = func;
  addr.register_num = reg;
  addr.enable = 1;

  pci_write_raw_reg(addr, value);
}

/// @brief Write a 32-bit register to the PCI configuration space.
///
/// This function writes the complete 32-bit register each time.
///
/// If invalid parameters are given, this function will panic.
///
/// @param bus PCI bus number. Can be any uint8_t value.
///
/// @param slot PCI slot number. Valid values are 0-31, inclusive.
///
/// @param func PCI device function number. Valid values are 0-7, inclusive.
///
/// @param reg PCI register.
///
/// @param value The value to write into the register
void pci_write_raw_reg (uint8_t bus, uint8_t slot, uint8_t func, PCI_REGS reg, uint32_t value)
{
  pci_write_raw_reg(bus, slot, func, static_cast<uint8_t>(reg), value);
}

/// @brief Instantiate the device at a given address.
///
/// Given a PCI device address, query the device to determine what it is and create a driver object for it.
///
/// @param bus PCI bus number. Can be any uint8_t value.
///
/// @param slot PCI slot number. Valid values are 0-31, inclusive.
///
/// @param func PCI device function number. Valid values are 0-7, inclusive.
///
/// @param parent The parent device of this PCI device.
///
/// @return If a driver can be loaded for the device, a pci_generic_device-derived object that drives the device.
///         nullptr if no suitable driver was loaded. Most devices will load pci_generic_device if no driver is found.
std::shared_ptr<pci_generic_device> pci_instantiate_device(uint8_t bus,
                                                           uint8_t slot,
                                                           uint8_t func,
                                                           std::shared_ptr<IDevice> parent)
{
  pci_reg_0 dev_reg0;
  pci_reg_2 dev_reg2;
  std::shared_ptr<pci_generic_device> new_device = nullptr;
  pci_address new_dev_addr;

  KL_TRC_ENTRY;

  dev_reg0.raw = pci_read_raw_reg(bus, slot, func, PCI_REGS::DEV_AND_VENDOR_ID);
  dev_reg2.raw = pci_read_raw_reg(bus, slot, func, PCI_REGS::CC_SC_PROG_IF_AND_REV_ID);

  new_dev_addr.raw = 0;
  new_dev_addr.bus = bus;
  new_dev_addr.device = slot;
  new_dev_addr.function = func;

  KL_TRC_TRACE(TRC_LVL::FLOW, "New PCI Device:\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Bus: ", bus, ", Slot: ", slot, ", Function: ", func, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Vendor ID: ", dev_reg0.vendor_id, ", Device ID: ", dev_reg0.device_id, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Class: ", dev_reg2.class_code, ", Subclass: ", dev_reg2.subclass, "\n");

  // First, look at the device and vendor IDs to see if there's a specific driver to load.
  switch (dev_reg0.vendor_id)
  {
  case virtio::VENDOR_ID:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Virtio device\n");

    if (((dev_reg0.device_id >= 0x1000) && (dev_reg0.device_id <= 0x1009)) ||
        ((dev_reg0.device_id >= 0x1041) && (dev_reg0.device_id <= 0x1058)))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found valid device ID: ", dev_reg0.device_id, "\n");
      new_device = virtio::instantiate_virtio_device(parent, new_dev_addr, dev_reg0.vendor_id, dev_reg0.device_id);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid virtio device ID found, fallback to generic devices\n");
    }
    break;

  default:
    KL_TRC_TRACE(TRC_LVL::FLOw, "Not found by vendor ID\n");
    break;
  }

  // If we didn't find a device specific driver, try looking for a generic one.
  if (!new_device)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No vendor specific driver, continue with generic choices\n");

    switch (dev_reg2.class_code)
    {
    case PCI_CLASS::MASS_STORE_CONTR:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Mass storage controller\n");
      switch(dev_reg2.subclass)
      {
        case PCI_SUBCLASS::IDE_CONTR:
          KL_TRC_TRACE(TRC_LVL::FLOW, "IDE Controller\n");
          {
            std::shared_ptr<ata::pci_controller> contr;
            if(dev::create_new_device(contr, parent, new_dev_addr))
            {
              new_device = contr;
            }
            else
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to construct new device\n");
            }
          }
          break;
      }
      break;

    case PCI_CLASS::SERIAL_BUS_CONTR:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Serial bus controller\n");
      switch (dev_reg2.subclass)
      {
        case PCI_SUBCLASS::USB_CONTR:
          KL_TRC_TRACE(TRC_LVL::FLOW, "USB controller\n");

          // It is safe to attempt to initialise the USB system more than once.
          usb::initialise_usb_system();

          switch (dev_reg2.prog_intface)
          {
            case 0x30:
              KL_TRC_TRACE(TRC_LVL::FLOW, "XHCI controller");
              {
                std::shared_ptr<usb::xhci::controller> contr;
                if (dev::create_new_device(contr, parent, new_dev_addr))
                {
                  new_device = contr;
                }
                else
                {
                  KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to construct new device\n");
                }
              }
              break;
          }

        break;
      }
      break;

    default:
      break;
    }
  }

  // Finally, fall back on the generic PCI device driver.
  if (!new_device)
  {
    std::shared_ptr<pci_generic_device> gen_dev;
    KL_TRC_TRACE(TRC_LVL::FLOW, "Fallback generic device\n");
    if (dev::create_new_device(gen_dev, parent, new_dev_addr))
    {
      new_device = std::make_shared<pci_generic_device>(new_dev_addr);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to construct new device\n");
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "New device @ ", new_device.get(), "\n");
  KL_TRC_EXIT;

  return new_device;
}

/// @brief Read a PCI Base Address Register.
///
/// This function is basically a shorthand around a pair of pci_read_raw_reg calls. It takes into account both 32- and
/// 64-bit Base Address Registers.
///
/// @param address PCI address of the device to query. The register number is ignored.
///
/// @param bar The number of the BAR to examine.
///
/// @return The address contained within the BAR. For 32-bit BARs, the upper 32-bits of the return value will be set to
///         zero. The type and prefetch fields are not masked - they are passed to the caller.
uint64_t pci_read_base_addr_reg(pci_address address, uint8_t bar)
{
  uint8_t reg;
  uint64_t addr{0};
  uint64_t part;

  KL_TRC_ENTRY;

  reg = static_cast<uint8_t>(PCI_REGS::BAR_0) + bar;
  addr = pci_read_raw_reg(address, reg);
  if (((addr & 0x000000004) == 4) && ((addr & 0x00000001) == 0))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "64-bit address register\n");
    part = pci_read_raw_reg(address, reg + 1);
    part <<= 32;

    addr |= part;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", addr, "\n");
  KL_TRC_EXIT;

  return addr;
}
