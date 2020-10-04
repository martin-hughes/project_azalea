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

  register_handler(SM_ATA_CMD_COMPLETE, DEF_CONVERT_HANDLER(ata_queued_command, handle_ata_cmd_response));

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

void generic_device::read(std::unique_ptr<msg::io_msg> msg)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Start block: ", msg->start, "\nNum blocks: ", msg->blocks, "\nBuffer: ",
               msg->buffer.get(), "\n");

  result = validate_request(msg);

  if (result == ERR_CODE::NO_ERROR)
  {
    if (dma_supported)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt DMA write\n");
      read_blocks_dma(std::move(msg));
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt PIO write\n");
      read_blocks_pio(std::move(msg));
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Request failed validation\n");
    msg->response = result;
    complete_io_request(std::move(msg));
  }

  KL_TRC_EXIT;
}

void generic_device::write(std::unique_ptr<msg::io_msg> msg)
{
  ERR_CODE result;

  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Start block: ", msg->start, "\nNum blocks: ", msg->blocks, "\nBuffer: ",
               msg->buffer.get(), "\n");

  result = validate_request(msg);

  if (result == ERR_CODE::NO_ERROR)
  {
    if (dma_supported)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt DMA write\n");
      write_blocks_dma(std::move(msg));
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt PIO write\n");
      write_blocks_pio(std::move(msg));
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Request failed validation\n");
    msg->response = result;
    complete_io_request(std::move(msg));
  }

  KL_TRC_EXIT;
}

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
/// @param msg The request being processed.
void generic_device::read_blocks_pio(std::unique_ptr<msg::io_msg> msg)
{
  COMMANDS read_cmd{COMMANDS::READ};

  KL_TRC_ENTRY;

  if (identity.lba_48 == 1)
  {
    read_cmd = COMMANDS::READ_EXT;
  }

  this->parent_controller->queue_command(this->controller_index,
                                         read_cmd,
                                         0,
                                         std::move(msg));

  KL_TRC_EXIT;
}

/// @brief Write a set of sectors in PIO mode.
///
/// @param msg The request being processed.
void generic_device::write_blocks_pio(std::unique_ptr<msg::io_msg> msg)
{
  COMMANDS write_cmd{COMMANDS::WRITE};

  KL_TRC_ENTRY;

  if (identity.lba_48 == 1)
  {
    write_cmd = COMMANDS::WRITE_EXT;
  }

  this->parent_controller->queue_command(this->controller_index,
                                         write_cmd,
                                         0,
                                         std::move(msg));

  flush_cache();

  KL_TRC_EXIT;
}

/// @brief Read a set of sectors in DMA mode.
///
/// @param msg The request being processed
///
/// @return A suitable error code.
void generic_device::read_blocks_dma(std::unique_ptr<msg::io_msg> msg)
{
  COMMANDS read_cmd{COMMANDS::READ_DMA};

  KL_TRC_ENTRY;

  if (identity.lba_48 == 1)
  {
    read_cmd = COMMANDS::READ_EXT_DMA;
  }

  msg->response = ERR_CODE::NO_ERROR;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Read ", msg->start, " -> +", msg->blocks, " blocks\n");

  this->parent_controller->queue_command(this->controller_index,
                                         read_cmd,
                                         0,
                                         std::move(msg));

  KL_TRC_EXIT;
}

/// @brief Write a set of sectors in DMA mode.
///
/// @param msg The request being processed.
///
/// @return A suitable error code.
void generic_device::write_blocks_dma(std::unique_ptr<msg::io_msg> msg)
{
  COMMANDS write_cmd{COMMANDS::WRITE_DMA};

  KL_TRC_ENTRY;

  if (identity.lba_48 == 1)
  {
    write_cmd = COMMANDS::WRITE_EXT_DMA;
  }

  msg->response = ERR_CODE::NO_ERROR;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Write ", msg->start, " -> +", msg->blocks, " blocks\n");

  this->parent_controller->queue_command(this->controller_index,
                                         write_cmd,
                                         0,
                                         std::move(msg));

  flush_cache();

  KL_TRC_EXIT;
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

  parent_controller->queue_command(controller_index, cmd, 0, nullptr);

  KL_TRC_EXIT;
}

/// @brief Confirm that a read or write request is valid
///
/// @param msg The request to confirm
///
/// @return ERR_CODE::NO_ERROR if everything is OK, an error code otherwise.
ERR_CODE generic_device::validate_request(std::unique_ptr<msg::io_msg> &msg)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};

  KL_TRC_ENTRY;

  if (!msg->buffer)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid write buffer\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if ((msg->start > this->number_of_sectors) || ((msg->start + msg->blocks) > this->number_of_sectors))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Number of sectors is out of range.\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (((msg->blocks > 0x10000) && (identity.lba_48 == 1)) || (msg->blocks > 0x100))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Too many blocks!\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (msg->blocks == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Too few blocks!\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else if (get_device_status() != OPER_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device has failed\n");
    result = ERR_CODE::DEVICE_FAILED;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Deal with the response from an ATA controller
///
/// This involves just passing back it to the party that made the original request, since there's no error handling yet
///
/// @param msg The response message
void generic_device::handle_ata_cmd_response(std::unique_ptr<ata_queued_command> msg)
{
  KL_TRC_ENTRY;

  if (msg->originator)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Got originator\n");
    ASSERT(msg->originator->response == ERR_CODE::NO_ERROR); // We don't have any error handling yet!

    complete_io_request(std::move(msg->originator));
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Nothing to do.\n");
  }

  KL_TRC_EXIT;
}
