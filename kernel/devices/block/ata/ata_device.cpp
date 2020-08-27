/// @file
/// @brief Simple, generic ATA device driver
//
// Known defects:
// - Amongst many others, does no error checking at all.
// - By keeping a shared_ptr to the parent controller, there's a pointer cycle - this isn't a problem until
//   hot-swappable ATA devices are implemented (if ever)

//#define ENABLE_TRACING

#include <stdint.h>

#include "ata_device.h"
#include "processor.h"
#include "timing.h"

#include <memory>

using namespace ata;

/// @brief Constructor for a Generic ATA Block Device.
///
/// @param parent Pointer to the controller this device is attached to.
///
/// @param drive_index The index of this drive according to the parent controller. This number has a controller-
///                    specific meaning and is effectively opaque to this device.
///
/// @param identity_buf Output from an earlier IDENTIFY command used to show that this device existed.
generic_device::generic_device(std::shared_ptr<generic_controller> parent, uint16_t drive_index, identify_cmd_output &identity_buf) :
    IDevice{"Generic ATA device", "ata", true},
    parent_controller{parent},
    controller_index{drive_index}
{
  KL_TRC_ENTRY;

  memcpy(&identity, &identity_buf, sizeof(identify_cmd_output));

  if (identity.lba_48 == 1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Supports LBA48\n");
    number_of_sectors = identity.num_logical_sectors_48;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Only LBA28\n");
    number_of_sectors = identity.num_logical_sectors_28;
  }

  dma_supported = calculate_dma_support();

  KL_TRC_TRACE(TRC_LVL::FLOW, "Sector count: ", number_of_sectors , "\n");

  set_device_status(OPER_STATUS::OK);

  KL_TRC_EXIT;
}

generic_device::~generic_device()
{

}

bool generic_device::start()
{
  set_device_status(OPER_STATUS::OK);
  return true;
}

bool generic_device::stop()
{
  set_device_status(OPER_STATUS::STOPPED);
  return true;
}

bool generic_device::reset()
{
  set_device_status(OPER_STATUS::STOPPED);
  return true;
}

uint64_t generic_device::num_blocks()
{
  KL_TRC_ENTRY;KL_TRC_EXIT;
  return this->number_of_sectors;
}

uint64_t generic_device::block_size()
{
  return SECTOR_LENGTH;
}

#if 0
ERR_CODE generic_device::read_blocks(uint64_t start_block,
                                     uint64_t num_blocks,
                                     void *buffer,
                                     uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Start block: ", start_block, "\nNum blocks: ", num_blocks, "\nBuffer: ",
      buffer, "\nBuffer length: ", buffer_length, "\n");

  if (buffer == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid receive buffer\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if ((start_block > this->number_of_sectors) || ((start_block + num_blocks) > this->number_of_sectors))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Number of sectors is out of range.\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (((num_blocks > 0x10000) && (identity.lba_48 == 1)) || (num_blocks > 0x100))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Too many blocks!\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (num_blocks == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Too few blocks!\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (buffer_length < (num_blocks * SECTOR_LENGTH))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Output buffer too short\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (get_device_status() != OPER_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device has failed\n");
    result = ERR_CODE::DEVICE_FAILED;
  }
  else
  {
    if (dma_supported)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt DMA read\n");
      result = read_blocks_dma(start_block, num_blocks, buffer, buffer_length);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt PIO read\n");
      result = read_blocks_pio(start_block, num_blocks, buffer, buffer_length);
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}

ERR_CODE generic_device::write_blocks(uint64_t start_block,
                                      uint64_t num_blocks,
                                      const void *buffer,
                                      uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::UNKNOWN;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Start block: ", start_block, "\nNum blocks: ", num_blocks, "\nBuffer: ",
      buffer, "\nBuffer length: ", buffer_length, "\n");

  if (buffer == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid write buffer\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if ((start_block > this->number_of_sectors) || ((start_block + num_blocks) > this->number_of_sectors))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Number of sectors is out of range.\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (((num_blocks > 0x10000) && (identity.lba_48 == 1)) || (num_blocks > 0x100))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Too many blocks!\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (num_blocks == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Too few blocks!\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (buffer_length < (num_blocks * SECTOR_LENGTH))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Output buffer too short\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (get_device_status() != OPER_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device has failed\n");
    result = ERR_CODE::DEVICE_FAILED;
  }
  else
  {
    if (dma_supported)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt DMA write\n");
      result = write_blocks_dma(start_block, num_blocks, buffer, buffer_length);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt PIO write\n");
      result = write_blocks_pio(start_block, num_blocks, buffer, buffer_length);
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}
#endif

/// @brief Calculate whether DMA is supported and configured on this device.
///
/// @return True if DMA is supported and ready to use on this device, false otherwise.
bool generic_device::calculate_dma_support()
{
  bool result{false};

  KL_TRC_ENTRY;

  if (identity.freefall_and_validity.word_88_valid == 1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Consider UDMA modes: ", identity.udma_modes.raw, "\n");
    if ((identity.udma_modes.modes_enabled & identity.udma_modes.modes_supported) != 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Some UDMA mode enabled\n");
      result = true;
    }
  }

  if (identity.freefall_and_validity.words_64_to_70_valid == 1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Consider MDMA modes: ", identity.multiword_dma_mode.raw, "\n");
    if ((identity.multiword_dma_mode.modes_enabled & identity.multiword_dma_mode.modes_supported) != 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Some Multiword DMA mode enabled\n");
      result = true;
    }
  }

  // Despite all of that, we can only support DMA transfers if our parent controller also supports them.
  result = result && parent_controller->dma_transfer_supported();

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Read a set of sectors in PIO mode.
///
/// @param start_block The first block to read.
///
/// @param num_blocks How many blocks to read.
///
/// @param buffer The buffer to write in to. It is assumed to have been checked for size already.
///
/// @param buffer_length The size of the buffer being written to.
///
/// @return A suitable error code.
ERR_CODE generic_device::read_blocks_pio(uint64_t start_block,
                                         uint64_t num_blocks,
                                         void *buffer,
                                         uint64_t buffer_length)
{
  ERR_CODE result;
  COMMANDS read_cmd{COMMANDS::READ};

  KL_TRC_ENTRY;

  if (identity.lba_48 == 1)
  {
    read_cmd = COMMANDS::READ_EXT;
  }

  if(this->parent_controller->issue_command(this->controller_index,
                                            read_cmd,
                                            0,
                                            num_blocks,
                                            start_block,
                                            buffer,
                                            buffer_length))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Good read\n");
    result = ERR_CODE::NO_ERROR;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read\n");
    result = ERR_CODE::STORAGE_ERROR;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  return result;
}

/// @brief Write a set of sectors in PIO mode.
///
/// @param start_block The first block to write.
///
/// @param num_blocks How many blocks to write.
///
/// @param buffer The buffer to read from. It is assumed to have been checked for size already.
///
/// @param buffer_length The size of the buffer being read from.
///
/// @return A suitable error code.
ERR_CODE generic_device::write_blocks_pio(uint64_t start_block,
                                          uint64_t num_blocks,
                                          const void *buffer,
                                          uint64_t buffer_length)
{
  ERR_CODE result;
  COMMANDS write_cmd{COMMANDS::WRITE};

  KL_TRC_ENTRY;

  if (identity.lba_48 == 1)
  {
    write_cmd = COMMANDS::WRITE_EXT;
  }

  if(this->parent_controller->issue_command(this->controller_index,
                                            write_cmd,
                                            0,
                                            num_blocks,
                                            start_block,
                                            const_cast<void *>(buffer),
                                            buffer_length))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Good write\n");
    flush_cache();
    result = ERR_CODE::NO_ERROR;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to write\n");
    result = ERR_CODE::STORAGE_ERROR;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  return result;
}

/// @brief Read a set of sectors in DMA mode.
///
/// @param start_block The first block to read.
///
/// @param num_blocks How many blocks to read.
///
/// @param buffer The buffer to write in to. It is assumed to have been checked for size already.
///
/// @param buffer_length The size of the buffer being written to.
///
/// @return A suitable error code.
ERR_CODE generic_device::read_blocks_dma(uint64_t start_block,
                                         uint64_t num_blocks,
                                         void *buffer,
                                         uint64_t buffer_length)
{
  ERR_CODE result = ERR_CODE::UNKNOWN;
  COMMANDS read_cmd{COMMANDS::READ_DMA};
  uint16_t blocks_this_part;
  uint64_t blocks_left = num_blocks;
  uint8_t *buffer_char_ptr = reinterpret_cast<uint8_t *>(buffer);

  KL_TRC_ENTRY;

  if (identity.lba_48 == 1)
  {
    read_cmd = COMMANDS::READ_EXT_DMA;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Read ", start_block, " -> +", num_blocks, " blocks\n");

  // This is the maximum number of 512-byte sectors our current host controller driver supports.
  if (num_blocks > 3840)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can only deal with slightly less than 2MB\n");
    result = ERR_CODE::TRANSFER_TOO_LARGE;
  }
  else
  {
    if (!parent_controller->start_prepare_dma_transfer(true, this->controller_index))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Parent controller rejected DMA start attempt\n");
      result = ERR_CODE::STORAGE_ERROR;
    }
    else
    {
      // This block queues up as many 64kB transfer blocks as needed to complete the whole transfer.
      while(blocks_left > 0)
      {
        // 128 is the number of 512-byte sectors in a 64kB transfer.
        blocks_this_part = (blocks_left > 128) ? 128 : blocks_left;

        if (!parent_controller->queue_dma_transfer_block(buffer_char_ptr, blocks_this_part * 512))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to queue part of a DMA transfer\n");
          break;
        }

        blocks_left -= blocks_this_part;
        buffer_char_ptr += (blocks_this_part * 512);
      }

      parent_controller->dma_transfer_blocks_queued();

      if((blocks_left == 0) && (this->parent_controller->issue_command(this->controller_index,
                                                                       read_cmd,
                                                                       0,
                                                                       num_blocks,
                                                                       start_block,
                                                                       buffer,
                                                                       buffer_length)))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Good read\n");
        result = ERR_CODE::NO_ERROR;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read\n");
        result = ERR_CODE::STORAGE_ERROR;
      }
    }

    result = ERR_CODE::NO_ERROR;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Write a set of sectors in DMA mode.
///
/// @param start_block The first block to write.
///
/// @param num_blocks How many blocks to write.
///
/// @param buffer The buffer to read from. It is assumed to have been checked for size already.
///
/// @param buffer_length The size of the buffer being read from.
///
/// @return A suitable error code.
ERR_CODE generic_device::write_blocks_dma(uint64_t start_block,
                                          uint64_t num_blocks,
                                          const void *buffer,
                                          uint64_t buffer_length)
{
  ERR_CODE result{ERR_CODE::UNKNOWN};
  COMMANDS write_cmd{COMMANDS::WRITE_DMA};
  uint16_t blocks_this_part;
  uint64_t blocks_left{num_blocks};
  uint8_t *buffer_char_ptr{reinterpret_cast<uint8_t *>(const_cast<void *>(buffer))};

  KL_TRC_ENTRY;

  if (identity.lba_48 == 1)
  {
    write_cmd = COMMANDS::WRITE_EXT_DMA;
  }

  // This is the maximum number of 512-byte sectors our current host controller driver supports.
  if (num_blocks > 3840)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can only deal with slightly less than 2MB\n");
    result = ERR_CODE::TRANSFER_TOO_LARGE;
  }
  else
  {
    if (!parent_controller->start_prepare_dma_transfer(false, this->controller_index))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Parent controller rejected DMA start attempt\n");
      result = ERR_CODE::STORAGE_ERROR;
    }
    else
    {
      // This block queues up as many 64kB transfer blocks as needed to complete the whole transfer.
      while(blocks_left > 0)
      {
        // 128 is the number of 512-byte sectors in a 64kB transfer.
        blocks_this_part = (blocks_left > 128) ? 128 : blocks_left;

        if (!parent_controller->queue_dma_transfer_block(buffer_char_ptr, blocks_this_part * 512))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to queue part of a DMA transfer\n");
          break;
        }

        blocks_left -= blocks_this_part;
        buffer_char_ptr += (blocks_this_part * 512);
      }

      parent_controller->dma_transfer_blocks_queued();

      if((blocks_left == 0) && (this->parent_controller->issue_command(this->controller_index,
                                                                       write_cmd,
                                                                       0,
                                                                       num_blocks,
                                                                       start_block,
                                                                       buffer_char_ptr,
                                                                       buffer_length)))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Good write\n");
        flush_cache();
        result = ERR_CODE::NO_ERROR;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to write\n");
        result = ERR_CODE::STORAGE_ERROR;
      }
    }

    result = ERR_CODE::NO_ERROR;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Flush the drive's cache to disk.
///
/// No error is reported if this isn't successful, but it might be useful to in future.
void generic_device::flush_cache()
{
  COMMANDS cmd{COMMANDS::FLUSH_CACHE};

  KL_TRC_ENTRY;

  if (identity.lba_48)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Send 48-bit flush command");
    cmd = COMMANDS::FLUSH_CACHE_EXT;
  }

  parent_controller->issue_command(controller_index, cmd, 0, 0, 0, nullptr, 0);

  KL_TRC_EXIT;
}
