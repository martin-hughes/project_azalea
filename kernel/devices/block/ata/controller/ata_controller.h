/// @file
/// @brief Implements a generic ATA Host Controller.
///
/// It will be necessary to subclass this to write, for example, PCI IDE or ADMA controller drivers.

#pragma once

#include <memory>
#include "types/common_messages.h"
#include "../ata_structures.h"

namespace ata
{

/// @brief Structure to store details of each of the ATA commands that are supported by Azalea.
///
struct command_properties
{
  uint16_t command_code; ///< The numerical code of the command.
  bool reads_sectors; ///< Does this command receive data into a buffer? Mutually exclusive with writes_sectors.
  bool writes_sectors; ///< Does this command write to disk? Mutually exclusive with reads_sectors.
  int32_t max_sectors; ///< The maximum number of sectors this command will accept. -1 = no limit.
  bool dma_command; ///< Does this command execute using DMA?
  bool lba_command; ///< Is this an LBA command (28- or 48-bit)?
  bool lba48_command; ///< Does this command require 48-bit addresses?
};

const uint16_t num_known_commands = 11; ///< The number of ATA commands in the 'known_commands' list.
extern const command_properties known_commands[num_known_commands];

/// @brief ATA commands.
///
/// The commands themselves are documented further in ATA8-ACS (ATA Command Set). The indicies are arbitrary.
enum class COMMANDS : uint16_t
{
  /// @cond
  READ = 0,
  READ_EXT = 1,
  READ_EXT_DMA = 2,
  WRITE = 3,
  WRITE_EXT = 4,
  WRITE_EXT_DMA = 5,
  READ_DMA = 6,
  WRITE_DMA = 7,
  IDENTIFY = 8,
  FLUSH_CACHE = 9,
  FLUSH_CACHE_EXT = 10,
  /// @endcond
};

/// @brief Class to hold details of an ATA command.
///
/// This is then dispatched to the controller by the system work queue mechanism.
class ata_queued_command : public msg::root_msg
{
public:
  ata_queued_command() : root_msg{SM_ATA_CMD} { };
  virtual ~ata_queued_command() = default;

  ata_queued_command(const ata_queued_command &) = delete;
  ata_queued_command(ata_queued_command &&) = delete;
  ata_queued_command &operator=(const ata_queued_command &) = delete;

  std::unique_ptr<msg::io_msg> originator; ///< The IO message responsible for causing this request.
  uint16_t drive_index; ///< The drive index issuing this request.
  COMMANDS command; ///< The command to execute.
  uint16_t features; ///< Any features flags to apply.
};

const uint16_t SECTOR_LENGTH = 512; ///< The expected length of a single sector.

/// @brief A Generic ATA Controller.
///
/// This class should expose all the functionality an ATA device, or users of an ATA device, might want to use.
/// However, on its own this class is insufficient, as it does not know how to send commands to a device - it is
/// necessary to use one of the subclasses, for example ata::pci_controller, for that.
class generic_controller
{
public:
  generic_controller() = default;
  virtual ~generic_controller() = default; ///< Normal destructor.

  // Generic Commands Section:

  virtual bool queue_command(uint16_t drive_index,
                             COMMANDS command,
                             uint16_t features,
                             std::unique_ptr<msg::io_msg> msg) = 0;

  /// @brief Does this controller support DMA-based transfers?
  ///
  /// @return True if the controller supports DMA based transfers, false otherwise. It is assumed that this state never
  ///         changes.
  virtual bool dma_transfer_supported() = 0;

  // ATA Commands Section:
  virtual bool cmd_identify(identify_cmd_output &identity, uint16_t drive_index);

protected:
  /// @brief Issue a command to the device.
  ///
  /// @param drive_index Which drive to execute the command on.
  ///
  /// @param command The command to execute.
  ///
  /// @param features The features word to send.
  ///
  /// @param count The count word to send. Usually the number of sectors to read/write.
  ///
  /// @param lba_addr The LBA address to send.
  ///
  /// @param buffer The buffer to write output to, or send inputs from.
  ///
  /// @return true if the command executed successfully, false otherwise.
  virtual bool issue_command(uint16_t drive_index,
                             COMMANDS command,
                             uint16_t features,
                             uint16_t count,
                             uint64_t lba_addr,
                             void *buffer) = 0;

};

}; // Namespace ata
