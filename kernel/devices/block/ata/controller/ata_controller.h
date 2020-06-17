/// @file
/// @brief Implements a generic ATA Host Controller.
///
/// It will be necessary to subclass this to write, for example, PCI IDE or ADMA controller drivers.

#pragma once

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
  /// @param buffer_len The number of bytes in buffer.
  ///
  /// @return true if the command executed successfully, false otherwise.
  virtual bool issue_command(uint16_t drive_index,
                             COMMANDS command,
                             uint16_t features,
                             uint16_t count,
                             uint64_t lba_addr,
                             void *buffer,
                             uint64_t buffer_len) = 0;

  /// @brief Does this controller support DMA-based transfers?
  ///
  /// @return True if the controller supports DMA based transfers, false otherwise. It is assumed that this state never
  ///         changes.
  virtual bool dma_transfer_supported() = 0;

  /// @brief Begin preparing for a DMA transfer.
  ///
  /// There are two parts to executing a DMA transfer on an ATA device. First, the controller needs to know the details
  /// of where the transfer is going to and from. Secondly, the ATA device needs to be commanded to begin the transfer.
  ///
  /// This function advises the controller of an upcoming DMA transfer. If the controller can only process one DMA
  /// transfer at a time then it may choose to block until there is an opportunity to begin another DMA transfer.
  ///
  /// @param is_read Is this a read from the device to RAM? If false, this is a write from RAM to device.
  ///
  /// @param drive_index The drive this read or write is occuring on
  ///
  /// @return True if the controller is now waiting for details of a DMA transfer to be given to it by
  ///         queue_dma_transfer_block, false otherwise.
  virtual bool start_prepare_dma_transfer(bool is_read, uint16_t drive_index) = 0;

  /// @brief Program part of a DMA transfer into the controller.
  ///
  /// DMA transfers can run in a scatter/gather mode, this function programs one element of the scattering or
  /// gathering.
  ///
  /// @param buffer The buffer to read/write bytes to/from
  ///
  /// @param bytes_this_block How many bytes to transfer from this buffer. If zero, 65536 bytes are transferred (as
  ///                         per the ATA Host Controller specification).
  ///
  /// @return True if this part of the transfer was queued successfully, false otherwise.
  virtual bool queue_dma_transfer_block(void *buffer, uint16_t bytes_this_block) = 0;

  /// @brief Finished programming DMA transfers in to the controller.
  ///
  /// The controller can now write the PRD table pointer to the controller.
  ///
  /// @return True.
  virtual bool dma_transfer_blocks_queued() = 0;

  // ATA Commands Section:
  virtual bool cmd_identify(identify_cmd_output &identity, uint16_t drive_index);
};

}; // Namespace ata
