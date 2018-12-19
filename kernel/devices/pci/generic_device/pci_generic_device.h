/// @file
/// @brief Generic PCI device driver. Other PCI devices may choose to inherit from this class, but it is not mandatory.

#pragma once

#include "devices/pci/pci_constants.h"
#include "devices/pci/pci_structures.h"
#include "devices/pci/pci_functions.h"

#include "devices/device_interface.h"
#include "system_tree/system_tree_leaf.h"

/// @brief A generic PCI device
///
/// Contains functions that may be useful to any PCI device. If a PCI device is detected in the system that doesn't
/// have a more appropriate driver for it, this class will manage it.
class pci_generic_device : public IDevice, public ISystemTreeLeaf, public IInterruptReceiver
{
public:
  pci_generic_device(pci_address address);
  virtual ~pci_generic_device() override;

protected:
  // Generic device commands.
  virtual const kl_string device_name() override;
  virtual DEV_STATUS get_device_status() override;

  // Interrupt receiver functions
  virtual bool handle_interrupt_fast(unsigned char interrupt_number) override;
  virtual void handle_interrupt_slow(unsigned char interrupt_number) override;
  virtual bool handle_translated_interrupt_fast(unsigned char interrupt_offset, unsigned char raw_interrupt_num) { return false; };
  virtual void handle_translated_interrupt_slow(unsigned char interrupt_offset, unsigned char raw_interrupt_num) { };

  // Generic capabilities commands.
  void zero_caps_list();
  void scan_caps();

  // MSI control commands.
  bool msi_configure(uint8_t interrupts_requested, uint8_t &interrupts_granted);
  bool msi_enable();
  bool msi_disable();

  // Member variables and structures.
  struct
  {
    pci::capability<void> pci_power_mgmt;
    pci::capability<void> agp;
    pci::capability<void> vital_prod_data;
    pci::capability<void> slot_ident;
    pci::capability<void> msi;
    pci::capability<void> compact_pci_hotswap;
    pci::capability<void> pci_x;
    pci::capability<void> hypertransport;
    pci::capability<void> vendor_specific_cap;
    pci::capability<void> debug_port;
    pci::capability<void> compact_pci_crc;
    pci::capability<void> pci_hotplug;
    pci::capability<void> pci_bridge_vendor_id;
    pci::capability<void> agp_8x;
    pci::capability<void> secure_device;
    pci::capability<void> pci_express;
    pci::capability<void> msi_x;
  } caps;

  pci_address _address;

  /// The lowest interrupt vector number we will receive. We can use this if we're configured to receive multiple
  /// interrupts to enable the recipient to know which vector we received relative to the lowest one, since most
  /// devices won't care about the actual vector number, but will care that they received the (e.g.) 10th vector in
  /// their allocation.
  uint8_t _base_interrupt_vector;

  /// How many interrupt vectors are allocated to this device? They must be contiguous after _base_interrupt_vector.
  ///
  uint8_t _num_allocated_vectors;
};