/// @file
/// @brief Declares a simple PCI-based ATA controller.

#pragma once

#include "klib/synch/kernel_locks.h"
#include "klib/synch/kernel_mutexes.h"
#include "devices/device_interface.h"
#include "devices/pci/generic_device/pci_generic_device.h"
#include "ata_controller.h"
#include "devices/block/ata/ata_device.h"

#include <memory>

namespace ata
{

#pragma pack (push, 1)

/// @brief Helper structure to interpret the PCI Class Code register.
///
union pci_class_code_reg
{
  uint32_t raw; ///< Raw Class Code Register
  struct
  {
    uint8_t pci_reserved; ///< Reserved for the PCI system.
    union
    {
      uint8_t raw_prof_if; ///< Raw Prog IF field.
      struct
      {
        uint8_t primary_pci_native : 1; ///< Does the primary controller support PCI native mode?
        uint8_t primary_mode_unlock : 1; ///< If set to 1, Native mode can be selected
        uint8_t secondary_pci_native : 1; ///< Does the secondary controller support PCI native mode?
        uint8_t secondary_mode_unlock : 1; ///< If set to 1, Native mode can be selected
        uint8_t reserved : 3; ///< Reserved
        uint8_t busmaster : 1; ///< Is bus mastering supported?
      }; ///< Helper structure
    }; ///< Prog IF field.
    uint8_t subclass_code; ///< PCI Device Subclass Code. Should be 1 or 5.
    uint8_t class_code; ///< PCI Device Class Code. Should be 1.
  }; ///< Helper structure
};
static_assert(sizeof(pci_class_code_reg) == 4);

/// @brief Bus Master IDE Status byte
///
/// AKA BMIS1 and BMIS2
struct bm_ide_status
{
  /// @cond
  union
  {
    uint8_t raw;
    struct
    {
      uint8_t bus_master_active : 1;
      uint8_t dma_error : 1;
      uint8_t interrupt_status : 1;
      uint8_t reserved_1 : 2;
      uint8_t master_dma_capable : 1;
      uint8_t slave_dma_capable : 1;
      uint8_t reserved_2 : 1;
    };
  };
  ///@endcond
};
static_assert(sizeof(bm_ide_status) == 1, "Incorrect packing of bm_ide_status");

#pragma pack (pop)

/// @brief ATA PIO port numbers.
///
/// These are well defined, so don't include in Doxygen.
enum ATA_PORTS
{
  /// @cond
  DATA_PORT = 0,
  FEATURES_PORT = 1,
  NUM_SECTORS_PORT = 2,
  LBA_LOW_BYTE = 3,
  LBA_MID_BYTE = 4,
  LBA_HIGH_BYTE = 5,
  DRIVE_SELECT_PORT = 6,
  COMMAND_STATUS_PORT = 7,
  /// @endcond
};

/// @brief IDE Bus Master Registers
///
/// Secondary registers are +0x08 from these.
enum BUS_MASTER_PORTS
{
  /// @cond
  COMMAND = 0,
  STATUS = 2,
  PRD_TABLE_ADDR_0 = 4,
  PRD_TABLE_ADDR_1 = 5,
  PRD_TABLE_ADDR_2 = 6,
  PRD_TABLE_ADDR_3 = 7,
  /// @endcond
};

/// @brief Stores port and other details about an attached device.
///
struct drive_details
{
  uint16_t base_cmd_regs_port; ///< The base port for the command block registers.
  uint16_t base_control_port; ///< The control port.
  uint16_t drive_select_byte; ///< When issuing a command, the value of the drive select byte, minus the LBA flag.
  uint8_t channel_number; ///< Which channel is the device attached to?
  std::shared_ptr<ata::generic_device> child_ptr; ///< Pointer to the driver object of the child device.
};

/// @brief One entry in an ATA Physical Region Descriptor Table
///
/// See the ATA host controller spec for more.
struct prd_table_entry
{
  /// @cond
  uint32_t region_phys_base_addr;
  uint16_t byte_count;
  uint16_t vendor_specific : 15;
  uint16_t end_of_table : 1;
  /// @endcond
};
static_assert(sizeof(prd_table_entry) == 8, "Sizeof prd_table_entry wrong.");

/// @brief The bus master command byte
///
/// See the ATA host controller spec for more.
union bus_master_cmd_byte
{
  /// @cond
  uint8_t raw;
  struct
  {
    uint8_t start : 1;
    uint8_t reserved_1 : 2;
    uint8_t is_write : 1;
    uint8_t reserved_2 : 4;
  };
  /// @endcond
};
static_assert(sizeof(bus_master_cmd_byte) == 1, "Sizeof bus_master_cmd_byte wrong.");

/// @brief Used for storing details of a transfer block that are useful to this driver.
///
/// Controller-specific details are stored in prd_table_entry.
struct dma_transfer_block_details
{
  void *buffer; ///< The buffer backing this transfer.
  uint16_t bytes_to_transfer; ///< The number of bytes to transfer. If 0, 65536 bytes are transferred.
};

/// @brief PCI-based ATA Host Controller
///
/// A simple ATA Host Controller based on the PCI bus. This device is based upon the specification "ATA/ATAPI Host
/// Adapters Standard (ATA – Adapter)" - which has has the codename T13/1510D - and is referred to here as the
/// "Controller spec".
class pci_controller : public pci_generic_device, public generic_controller
{
public:
  pci_controller(pci_address address);
  virtual ~pci_controller() = default; ///< Normal destructor.

protected:
  static const uint16_t MAX_CHANNEL = 2; ///< How many channels are supported on this controller
  static const uint16_t DRIVES_PER_CHAN = 2; ///< How many drives are supported per channel?
  static const uint16_t MAX_DRIVE_IDX = MAX_CHANNEL * DRIVES_PER_CHAN; ///< The total number of supported drives.

  /// Stores details of each known drive on this controller.
  ///
  drive_details drives_by_index_num[MAX_DRIVE_IDX]
    {
      { 0x1F0, 0x3F4, 0xA0, 0, nullptr }, // Primary channel, master device.
      { 0x1F0, 0x3F4, 0xB0, 0, nullptr }, // Primary channel, slave device.
      { 0x170, 0x376, 0xA0, 1, nullptr }, // Secondary channel, master device.
      { 0x170, 0x376, 0xB0, 1, nullptr }  // Secondary channel, slave
    };

  uint16_t channel_irq_nums[MAX_CHANNEL]{14, 15}; ///< Which IRQs are connected to which channel.
  uint16_t bus_master_base_port{0}; ///< The base port of this controller's bus mastering registers.

  kernel_spinlock cmd_spinlock; ///< Prevents getting our commands confused by serialising access to the drives.
  klib_mutex dma_mutex; ///< Mutex to help queue DMA transfers, since only one can execute at a time.

  static const uint16_t max_prd_table_entries = 31; ///< The maximum number of transfers that fit into the buffer above
  uint64_t buffer_phys_addr{0}; ///< Physical address of a memory buffer known to conform to DMA requirements.
  uint64_t buffer_virt_addr{0}; ///< Virtual address of the same memory buffer.
  prd_table_entry *prd_table{nullptr}; ///< PRD table for DMA transfers
  uint16_t num_prd_table_entries{0}; ///< Number of entries in PRD table for this transfer.
  bool dma_transfer_is_read{false}; ///< Is the next DMA operation a read (true) or write (false)?
  volatile bool interrupt_on_chan[MAX_CHANNEL]{false, false}; ///< An interrupt has occurred on the given channel.
  /// Stores extra information about transfers.
  ///
  dma_transfer_block_details transfer_block_details[max_prd_table_entries];

public:
  // Overrides of pci_generic_device:

  // Overrides of generic_controller:
  virtual bool start_prepare_dma_transfer(bool is_read, uint16_t drive_index) override;
  virtual bool queue_dma_transfer_block(void *buffer, uint16_t bytes_this_block) override;
  virtual bool issue_command(uint16_t drive_index,
                             COMMANDS command,
                             uint16_t features,
                             uint16_t count,
                             uint64_t lba_addr,
                             void *buffer,
                             uint64_t buffer_len) override;

protected:
  // Our own member functions.
  void determine_ports();
  void write_ata_cmd_port(uint16_t drive_index, ATA_PORTS port, uint8_t value);
  uint8_t read_ata_cmd_port(uint16_t drive_index, ATA_PORTS port);
  bool wait_for_cmd_completion(uint16_t drive_index);
  bool poll_wait_for_drive_not_busy(uint16_t drive_index);
  void pio_read_sectors_to_buffer(uint16_t drive_index,
                                  uint16_t sectors,
                                  unsigned char *buffer,
                                  uint64_t buffer_length);
  void dma_read_sectors_to_bufers();
  bool pio_write_sectors_to_drive(uint16_t drive_index,
                                  uint16_t sectors,
                                  uint8_t *buffer,
                                  uint64_t buffer_length);
  bool continue_with_dma_setup();

  // Overrides of pci_generic_device:
  virtual bool handle_translated_interrupt_fast(unsigned char interrupt_offset,
                                                unsigned char raw_interrupt_num) override;
  virtual void handle_translated_interrupt_slow(unsigned char interrupt_offset,
                                                unsigned char raw_interrupt_num) override;

  // Bus master control
  void write_prd_table_addr(uint32_t address, uint16_t channel);
  void set_bus_master_direction(bool is_read, uint16_t channel);
  void start_bus_master(uint16_t channel);
  void stop_bus_master(uint16_t channel);

  void write_bus_master_reg(BUS_MASTER_PORTS port, uint16_t channel, uint8_t val);
  uint8_t read_bus_master_reg(BUS_MASTER_PORTS port, uint16_t channel);
};

}; // Namespace ata
