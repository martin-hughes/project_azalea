/// @file
/// @brief Functions useful to all code dealing with PCI devices.

#pragma once

#include <stdint.h>
#include <memory>

#include "devices/pci/pci_constants.h"
#include "devices/pci/pci_structures.h"

class pci_generic_device;

uint32_t pci_read_raw_reg (uint8_t bus, uint8_t slot, uint8_t func, PCI_REGS reg);
uint32_t pci_read_raw_reg (uint8_t bus, uint8_t slot, uint8_t func, uint8_t reg);
uint32_t pci_read_raw_reg (pci_address address, PCI_REGS reg);
uint32_t pci_read_raw_reg (pci_address address, uint8_t reg);
uint32_t pci_read_raw_reg (pci_address address);

void pci_write_raw_reg (uint8_t bus, uint8_t slot, uint8_t func, PCI_REGS reg, uint32_t value);
void pci_write_raw_reg (uint8_t bus, uint8_t slot, uint8_t func, uint8_t reg, uint32_t value);
void pci_write_raw_reg (pci_address address, PCI_REGS reg, uint32_t value);
void pci_write_raw_reg (pci_address address, uint8_t reg, uint32_t value);
void pci_write_raw_reg (pci_address address, uint32_t value);

std::shared_ptr<pci_generic_device> pci_instantiate_device(uint8_t bus, uint8_t slot, uint8_t func);