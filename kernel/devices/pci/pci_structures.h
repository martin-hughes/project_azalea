/// @file
/// @brief Structures relevant to all PCI devices.

#pragma once

#include <stdint.h>

/// @brief Stucture storing address of a single PCI device
///
struct pci_address
{
/// @cond
  union
  {
    uint32_t raw;
    struct
    {
      uint32_t reserved_1 : 2;
      uint32_t register_num : 6;
      uint32_t function : 3;
      uint32_t device : 5;
      uint32_t bus : 8;
      uint32_t reserved_2 : 7;
      uint32_t enable : 1;
    };
  };
/// @endcond
};
static_assert(sizeof(pci_address) == sizeof(uint32_t), "Bad packing of pci_address");

/// @brief Standard format of PCI device register 0.
///
struct pci_reg_0
{
/// @cond
  union
  {
    uint32_t raw;
    struct
    {
      uint16_t vendor_id;
      uint16_t device_id;
    };
  };
/// @endcond
};
static_assert(sizeof(pci_reg_0) == sizeof(uint32_t), "Bad packing of pci_reg_0");

/// @brief Standard format of PCI device register 1.
///
struct pci_reg_1
{
/// @cond
  union
  {
    uint32_t raw;
    struct
    {
      union
      {
        uint16_t command;
        struct
        {
          uint16_t io_space_enable : 1;
          uint16_t mem_space_enable : 1;
          uint16_t bus_master_enable : 1;
          uint16_t special_cycles_enable : 1;
          uint16_t mem_write_and_inval_en : 1;
          uint16_t vga_pal_snoop_enable : 1;
          uint16_t parity_err_response : 1;
          uint16_t reserved_1 : 1;
          uint16_t sys_err_enable : 1;
          uint16_t fast_b2b_enable : 1;
          uint16_t interrupt_disable : 1;
          uint16_t reserved_2 : 5;
        };
      };
      union
      {
        uint16_t status;
        struct
        {
          uint16_t reserved_3 : 3;
          uint16_t interrupt_status : 1;
          uint16_t new_caps_list : 1;
          uint16_t mhz66_capable : 1;
          uint16_t reserved_4 : 1;
          uint16_t fast_b2b_capable : 1;
          uint16_t master_data_parity_err : 1;
          uint16_t devsel_timing : 2;
          uint16_t signaled_target_abort : 1;
          uint16_t received_target_abort : 1;
          uint16_t received_master_abort : 1;
          uint16_t signaled_sys_err : 1;
          uint16_t parity_err_detected : 1;
        };
      };
    };
  };
/// @endcond
};
static_assert(sizeof(pci_reg_1) == sizeof(uint32_t), "Bad packing of pci_reg_1");

/// @brief Standard format of PCI device register 2.
///
struct pci_reg_2
{
/// @cond
  union
  {
    uint32_t raw;
    struct
    {
      uint8_t revision_id;
      uint8_t prog_intface;
      uint8_t subclass;
      uint8_t class_code;
    };
  };
/// @endcond
};
static_assert(sizeof(pci_reg_2) == sizeof(uint32_t), "Bad packing of pci_reg_2");

/// @brief Standard format of PCI device register 3.
///
struct pci_reg_3
{
/// @cond
  union
  {
    uint32_t raw;
    struct
    {
      uint8_t cache_line_size;
      uint8_t latency_timer;
      uint8_t header_type;
      uint8_t bist;
    };
  };
/// @endcond
};
static_assert(sizeof(pci_reg_3) == sizeof(uint32_t), "Bad packing of pci_reg_3");

/// @brief Standard format of PCI device register 13.
///
struct pci_reg_13
{
/// @cond
  union
  {
    uint32_t raw;
    struct
    {
      uint8_t caps_offset;
      uint8_t reserved[3];
    };
  };
/// @endcond
};
static_assert(sizeof(pci_reg_13) == sizeof(uint32_t), "Bad packing of pci_reg_13");

/// @brief Standard format of PCI register 15:
struct pci_reg_15
{
  /// @cond
  union
  {
    uint32_t raw;
    struct
    {
      uint8_t interrupt_line;
      uint8_t interrupt_pin;
      union
      {
        uint16_t bridge_control;
        struct
        {
          uint8_t min_grant;
          uint8_t max_latency;
        };
      };
    };
  };
  /// @endcond
};
static_assert(sizeof(pci_reg_15) == sizeof(uint32_t), "Bad packing of pci_reg_15");

/// @brief Standard header of PCI device capabilities structures.
///
struct pci_cap_header
{
/// @cond
  union
  {
    uint32_t raw;
    struct
    {
      uint8_t cap_label;
      uint8_t next_cap_offset;
      uint16_t cap_data;
    };
  };
/// @encond
};
static_assert(sizeof(pci_cap_header) == sizeof(uint32_t), "Bad packing of pci_cap_header");

/// @brief Header of PCI device MSI capabilities.
///
struct pci_msi_cap_header
{
/// @cond
  union
  {
    uint32_t raw;
    struct
    {
      uint8_t cap_label;
      uint8_t next_cap_offset;
      uint16_t msi_enable : 1;
      uint16_t multiple_msg_capable : 3;
      uint16_t multiple_msg_enable : 3;
      uint16_t cap_64_bit_addr : 1;
      uint16_t cap_per_vector_mask : 1;
      uint16_t reserved : 7;
    };
  };
/// @endcond
};
static_assert(sizeof(pci_msi_cap_header) == sizeof(pci_cap_header), "Bad packing of pci_msi_cap_header");

namespace pci
{
  /// @brief Structure to contain any PCI device capability.
  ///
  template <typename T> struct capability
  {
    bool supported; ///< Is this capability supported?
    uint8_t offset; ///< What is the offset in the list of capabilities?
    T *base_mem_address; ///< What is the memory address of the capablity structure.
  };
};