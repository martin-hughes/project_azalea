/// @file
/// @brief Declares an object backing an ACPI PCI IRQ Mapping device.

#pragma once

#include <memory>

class kl_string;

/// @brief A pseudo-device to help interpret ACPI IRQ mapping tables.
///
/// If this device is found in the ACPI device tables then it is used to calculate the mapping between PCI IRQ pins and
/// processor interrupts.
class pci_irq_link_device
{
protected:
  pci_irq_link_device(ACPI_HANDLE dev_handle);

public:
  virtual ~pci_irq_link_device() = default;

  static std::shared_ptr<pci_irq_link_device> create(kl_string &pathname, ACPI_HANDLE obj_handle);

  uint16_t get_interrupt();

protected:
  uint16_t chose_interrupt(uint16_t interrupt_count, void *choices, uint8_t bytes_per_int);

  uint16_t chosen_interrupt; ///< The interrupt that this device has chosen to use.
};
