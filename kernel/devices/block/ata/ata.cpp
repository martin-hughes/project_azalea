/// @brief Simple, generic ATA device driver

//#define ENABLE_TRACING

#include <stdint.h>

#include "klib/klib.h"
#include "ata.h"
#include "processor/processor.h"
#include "processor/timing/timing.h"

#include <memory>

namespace
{
const uint16_t SECTOR_LENGTH = 512;

kernel_spinlock ata_spinlock = 0;
}

struct status_byte
{
  uint8_t error_flag :1;
  uint8_t reserved :2;
  uint8_t data_ready_flag :1;
  uint8_t overlapped_service_flag :1;
  uint8_t drive_fault_flag :1;
  uint8_t drive_ready_flag :1;
  uint8_t busy_flag :1;
};

static_assert(sizeof(status_byte) == 1, "Incorrect packing of status_byte");

generic_ata_device::generic_ata_device(uint16_t base_port, bool master) :
    _name("Generic ATA device"),
    _base_port(base_port),
    _master(master),
    _status(DEV_STATUS::FAILED),
    supports_lba48(false),
    number_of_sectors(0)
{
  KL_TRC_ENTRY;

  unsigned char result;

  std::unique_ptr<uint16_t[]> identify_buffer(new uint16_t[SECTOR_LENGTH / sizeof(uint16_t)]);

  // Send an IDENTIFY command and read the results
  klib_synch_spinlock_lock(ata_spinlock);
  write_ata_cmd_port(DRIVE_SELECT_PORT, (this->_master ? 0xA0 : 0xB0));
  write_ata_cmd_port(NUM_SECTORS_PORT, 0);
  write_ata_cmd_port(LBA_LOW_BYTE, 0);
  write_ata_cmd_port(LBA_MID_BYTE, 0);
  write_ata_cmd_port(LBA_HIGH_BYTE, 0);
  write_ata_cmd_port(COMMAND_STATUS_PORT, ATA_IDENTIFY);
  result = read_ata_cmd_port(COMMAND_STATUS_PORT);

  if (result == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No device found\n");
    this->_status = DEV_STATUS::NOT_PRESENT;
  }
  else
  {
    if (!wait_and_poll())
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Device is failed\n");
      this->_status = DEV_STATUS::FAILED;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Reading results\n");
      read_sector_to_buffer((unsigned char *)identify_buffer.get(), SECTOR_LENGTH);

      supports_lba48 = ((identify_buffer[83] & (1 << 10)) != 0);

      if (supports_lba48)
      {
        kl_memcpy(&identify_buffer[100], &number_of_sectors, 64);
        KL_TRC_TRACE(TRC_LVL::FLOW, "Supports LBA48 with ", number_of_sectors, " sectors\n");
      }
      else
      {
        kl_memcpy(&identify_buffer[60], &number_of_sectors, 32);
        KL_TRC_TRACE(TRC_LVL::FLOW, "Supports LBA24 with ", number_of_sectors, " sectors\n");
      }

      _status = DEV_STATUS::OK;
    }
  }

  proc_write_port(0x3F6, 1, 8);

  klib_synch_spinlock_unlock(ata_spinlock);

  KL_TRC_EXIT;
}

generic_ata_device::~generic_ata_device()
{

}

const kl_string generic_ata_device::device_name()
{
  KL_TRC_ENTRY;KL_TRC_EXIT;
  return this->_name;
}

DEV_STATUS generic_ata_device::get_device_status()
{
  KL_TRC_ENTRY;KL_TRC_EXIT;
  return this->_status;
}

uint64_t generic_ata_device::num_blocks()
{
  KL_TRC_ENTRY;KL_TRC_EXIT;
  return this->number_of_sectors;
}

uint64_t generic_ata_device::block_size()
{
  return SECTOR_LENGTH;
}

ERR_CODE generic_ata_device::read_blocks(uint64_t start_block,
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
  else if (((num_blocks > 0x10000) && (this->supports_lba48)) || (num_blocks > 0x100))
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
  else if (this->_status != DEV_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device has failed\n");
    result = ERR_CODE::DEVICE_FAILED;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Looks good for an attempt to read\n");

    klib_synch_spinlock_lock(ata_spinlock);

    // Do an LBA48 read if possible.
    if (this->supports_lba48)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt an LBA48 read\n");

      // Select a drive in LBA48 mode.
      write_ata_cmd_port(DRIVE_SELECT_PORT, this->_master ? 0x40 : 0x50);

      // Write the various address parts:
      write_ata_cmd_port(NUM_SECTORS_PORT, ((num_blocks & 0xFF00) >> 8)); // Sector count high byte
      write_ata_cmd_port(LBA_LOW_BYTE, ((start_block & 0x0000FF000000) >> 24)); // LBA byte 4
      write_ata_cmd_port(LBA_MID_BYTE, ((start_block & 0x00FF00000000) >> 32)); // LBA byte 5
      write_ata_cmd_port(LBA_HIGH_BYTE, ((start_block & 0xFF0000000000) >> 40)); // LBA byte 6
      write_ata_cmd_port(NUM_SECTORS_PORT, (num_blocks & 0xFF)); // Sector count low byte
      write_ata_cmd_port(LBA_LOW_BYTE, (start_block & 0x0000000000FF)); // LBA byte 1
      write_ata_cmd_port(LBA_MID_BYTE, ((start_block & 0x00000000FF00) >> 8)); // LBA byte 2
      write_ata_cmd_port(LBA_HIGH_BYTE, ((start_block & 0x000000FF0000) >> 16)); // LBA byte 3

      // Send the actual command:
      write_ata_cmd_port(COMMAND_STATUS_PORT, ATA_READ_EXT);

      // Wait for the results:
      if (!wait_and_poll())
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Something failed\n");
        this->_status = DEV_STATUS::FAILED;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt read in LBA24 mode\n");

      write_ata_cmd_port(DRIVE_SELECT_PORT, this->_master ? 0xE0 : 0xF0);

      // Write the various address parts:
      write_ata_cmd_port(NUM_SECTORS_PORT, (num_blocks & 0xFF)); // Sector count low byte
      write_ata_cmd_port(LBA_LOW_BYTE, (start_block & 0x0000000000FF)); // LBA byte 1
      write_ata_cmd_port(LBA_MID_BYTE, ((start_block & 0x00000000FF00) >> 8)); // LBA byte 2
      write_ata_cmd_port(LBA_HIGH_BYTE, ((start_block & 0x000000FF0000) >> 16)); // LBA byte 3

      // Send the actual command:
      write_ata_cmd_port(COMMAND_STATUS_PORT, ATA_READ);
    }

    // And do the actual reading.
    for (unsigned int i = 0; i < num_blocks; i++)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Reading sector: ", (unsigned long)i, "\n");

      // Wait for 400ns, and then wait for seeking to finish.
      time_stall_process(400);
      if (!wait_and_poll())
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Something failed\n");
        this->_status = DEV_STATUS::FAILED;
      }

      read_sector_to_buffer(reinterpret_cast<unsigned char *>(buffer) + (i * SECTOR_LENGTH),
                            buffer_length - (i * SECTOR_LENGTH));
    }

    klib_synch_spinlock_unlock(ata_spinlock);

    if (this->_status == DEV_STATUS::OK)
    {
      result = ERR_CODE::NO_ERROR;
    }
    else
    {
      result = ERR_CODE::DEVICE_FAILED;
    }
  }

  KL_TRC_EXIT;
  return result;
}

ERR_CODE generic_ata_device::write_blocks(uint64_t start_block,
                                          uint64_t num_blocks,
                                          void *buffer,
                                          uint64_t buffer_length)
{
  return ERR_CODE::INVALID_OP;
}

void generic_ata_device::write_ata_cmd_port(ATA_PORTS port, uint8_t value)
{
  KL_TRC_ENTRY;

  ASSERT(port != ATA_PORTS::DATA_PORT);
  proc_write_port(this->_base_port + port, value, 8);

  KL_TRC_EXIT;
}

uint8_t generic_ata_device::read_ata_cmd_port(ATA_PORTS port)
{
  uint8_t res;

  KL_TRC_ENTRY;
  ASSERT(port != ATA_PORTS::DATA_PORT);
  res = static_cast<uint8_t>(proc_read_port(this->_base_port + port, 8));

  KL_TRC_EXIT;
  return res;
}

bool generic_ata_device::wait_and_poll()
{
  KL_TRC_ENTRY;

  status_byte result;
  uint8_t result_c;
  bool ret;

  // Do 4 dummy reads to flush the status
  read_ata_cmd_port(COMMAND_STATUS_PORT);
  read_ata_cmd_port(COMMAND_STATUS_PORT);
  read_ata_cmd_port(COMMAND_STATUS_PORT);
  read_ata_cmd_port(COMMAND_STATUS_PORT);

  do
  {
    result_c = read_ata_cmd_port(COMMAND_STATUS_PORT);
    kl_memcpy(&result_c, &result, 1);
  } while ((result.busy_flag == 1)
           || ((result.error_flag = 0)
           && (result.data_ready_flag == 0)
           && (result.drive_fault_flag == 0)));

  if ((result.error_flag == 1) || (result.drive_fault_flag == 1))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Drive failed\n");
    ret = false;
  }
  else
  {
    ret = true;
  }

  return ret;

  KL_TRC_EXIT;
}

void generic_ata_device::read_sector_to_buffer(unsigned char *buffer, uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  uint16_t *int_buffer = reinterpret_cast<uint16_t *>(buffer);

  ASSERT(buffer_length >= SECTOR_LENGTH);
  for (uint16_t i = 0; i < SECTOR_LENGTH; i += 2)
  {
    int_buffer[i / 2] = proc_read_port(this->_base_port + DATA_PORT, 16);
  }

  KL_TRC_EXIT;
}
