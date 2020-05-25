/// @file
/// @brief Generic PCI device driver. Other PCI devices may choose to inherit from this class, but it is not mandatory.

#pragma once

#include <string>
#include <list>

#include "devices/pci/pci_constants.h"
#include "devices/pci/pci_structures.h"
#include "devices/pci/pci_functions.h"

#include "devices/device_interface.h"
#include "system_tree/system_tree_leaf.h"

/// @brief A generic PCI device
///
/// Contains functions that may be useful to any PCI device. If a PCI device is detected in the system that doesn't
/// have a more appropriate driver for it, this class will manage it.
class pci_generic_device : public IDevice, public IInterruptReceiver
{
public:
  pci_generic_device(pci_address address, const std::string human_name, const std::string dev_name);
  pci_generic_device(pci_address address);
  virtual ~pci_generic_device() override;

  // Overrides of IDevice.
  bool start() override;
  bool stop() override;
  bool reset() override;

protected:

  // Interrupt receiver functions
  virtual uint16_t compute_irq_for_pin(uint8_t pin); // Note that this is implemented in pci_legacy_interrupts.cpp.
  virtual bool handle_interrupt_fast(uint8_t interrupt_number) override;
  virtual void handle_interrupt_slow(uint8_t interrupt_number) override;

  /// @brief A version of handle_interrupt_fast() where a translated interrupt number is given.
  ///
  /// A translated interrupt number is given relative to the number of the lowest interrupt allocated to this device.
  ///
  /// @param interrupt_offset The interrupt number relative to the lowest interrupt allocated to this device.
  ///
  /// @param raw_interrupt_num The original interrupt number, untranslated.
  ///
  /// @return True if the handler needs to execute the slow path as well, false if no further processing is needed.
  virtual bool handle_translated_interrupt_fast(uint8_t interrupt_offset,
                                                uint8_t raw_interrupt_num) { return false; };


  /// @brief A version of handle_interrupt_slow() where a translated interrupt number is given.
  ///
  /// A translated interrupt number is given relative to the number of the lowest interrupt allocated to this device.
  ///
  /// @param interrupt_offset The interrupt number relative to the lowest interrupt allocated to this device.
  ///
  /// @param raw_interrupt_num The original interrupt number, untranslated.
  virtual void handle_translated_interrupt_slow(uint8_t interrupt_offset,
                                                uint8_t raw_interrupt_num) { };

  // Generic capabilities commands.
  void zero_caps_list();
  void scan_caps();
  virtual bool read_capability_block(pci::capability<void> &cap, void *buffer, uint16_t buffer_length);

  // MSI control commands.
  bool msi_configure(uint8_t interrupts_requested, uint8_t &interrupts_granted);
  bool msi_enable();
  bool msi_disable();

  // Bus mastering commands.
  bool bm_enable();
  bool bm_disable();
  bool bm_enabled();

  // Member variables and structures.
  struct
  {
    pci::capability<void> pci_power_mgmt; ///< PCI power management capability.
    pci::capability<void> agp; ///< AGP capability.
    pci::capability<void> vital_prod_data; ///< Vital product data capability.
    pci::capability<void> slot_ident; ///< Slot identification capability.
    pci::capability<void> msi; ///< Message Signalled Interrupt capability.
    pci::capability<void> compact_pci_hotswap; ///< Compact PCI hotswap capability.
    pci::capability<void> pci_x; ///< PCI-X capability.
    pci::capability<void> hypertransport; ///< Hypertransport capability.
    pci::capability<void> debug_port; ///< Debug-port capability.
    pci::capability<void> compact_pci_crc; ///< Compact-PCI CRC capability.
    pci::capability<void> pci_hotplug; ///< PCI hotplug capability.
    pci::capability<void> pci_bridge_vendor_id; ///< PCI Bridge capability.
    pci::capability<void> agp_8x; ///< AGP 8x capability.
    pci::capability<void> secure_device; ///< Secure device capability.
    pci::capability<void> pci_express; ///< PCI Express capability.
    pci::capability<void> msi_x; ///< MSI-X capability.

    std::list<pci::capability<void>> vendor_specific; ///< Vendor-specific capabilities.
  } caps; ///< Capabilities of this device.

  pci_address _address; ///< The address of this device.

  /// The lowest interrupt vector number we will receive. We can use this if we're configured to receive multiple
  /// interrupts to enable the recipient to know which vector we received relative to the lowest one, since most
  /// devices won't care about the actual vector number, but will care that they received the (e.g.) 10th vector in
  /// their allocation.
  uint8_t _base_interrupt_vector;

  /// How many interrupt vectors are allocated to this device? They must be contiguous after _base_interrupt_vector.
  ///
  uint8_t _num_allocated_vectors;
};
