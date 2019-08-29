/// @file
/// @brief Implements a PCI-based ATA Host Controller.
//
// Known defects:
// - Amongst many others, does no error checking at all.
// - We should definitely be retrieving error codes from the drive after issuing commands!
// - We only allow queueing slightly less than 2MB of DMA transfer at once.
// - DMA transfers always go into a bounce buffer.
// - We don't check to see if the transfer fails, the drive just becomes unusable - there are some trace comments, but
//   they don't do anything.
// - The interrupt code assumes the two channels have different interrupt numbers, which may not be true.
// - We only support one DMA transfer by channel, but some systems do support one per drive apparently.
// - - actually, the DMA mutex locks us to one per controller, but this could be changed easily enough.
// - There's no checking that DMA transfers are queued properly before beginning the transfer.

//#define ENABLE_TRACING

#include "ata_pci_controller.h"
#include "devices/block/ata/ata_structures.h"
#include "processor/timing/timing.h"
#include "klib/klib.h"

using namespace ata;

// This declares a device used in entry.cpp to load the init program from.
extern ata::generic_device *first_hdd; ///< TEMPORARY global variable.

/// @brief Normal constructor for PCI ATA Host Controllers
///
/// @param address The address of this controller on the PCI bus.
pci_controller::pci_controller(pci_address address) :
  pci_generic_device{address, "PCI ATA Host Controller"}
{
  identify_cmd_output ident;

  KL_TRC_ENTRY;

  klib_synch_spinlock_init(cmd_spinlock);
  klib_synch_mutex_init(dma_mutex);

  // Determine which I/O ports are in use.
  determine_ports();

  // Attempt to identify any devices attached to this controller.
  for (uint16_t i = 0; i < MAX_DRIVE_IDX; i++)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Examine device ", i, "\n");
    if(cmd_identify(ident, i))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found device\n");
      drives_by_index_num[i].child_ptr = std::make_shared<ata::generic_device>(this, i, ident);
    }
  }

  // Give the first device special treatment (for now)
  first_hdd = drives_by_index_num[0].child_ptr.get();

  proc_register_irq_handler(channel_irq_nums[0], this);
  if (channel_irq_nums[1] != channel_irq_nums[0])
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Also register second channel\n");
    proc_register_irq_handler(channel_irq_nums[1], this);
  }

  current_dev_status = DEV_STATUS::OK;

  KL_TRC_EXIT;
};

/// @brief Determine which I/O ports the child devices will respond to.
///
void pci_controller::determine_ports()
{
  pci_class_code_reg cc;
  pci_reg_15 interrupt_reg;

  KL_TRC_ENTRY;

  cc.raw = pci_read_raw_reg(_address, PCI_REGS::CC_SC_PROG_IF_AND_REV_ID);
  interrupt_reg.raw = pci_read_raw_reg(_address, PCI_REGS::LATS_AND_INTERRUPTS);

  if (cc.primary_pci_native)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Primary device is PCI Native\n");
    drives_by_index_num[0].base_cmd_regs_port = pci_read_raw_reg(_address, PCI_REGS::BAR_0) & 0xFFF0;
    drives_by_index_num[1].base_cmd_regs_port = drives_by_index_num[0].base_cmd_regs_port;

    drives_by_index_num[0].base_control_port = pci_read_raw_reg(_address, PCI_REGS::BAR_1) & 0xFFF0;
    drives_by_index_num[1].base_control_port = drives_by_index_num[0].base_control_port;

    channel_irq_nums[0] = compute_irq_for_pin(interrupt_reg.interrupt_pin - 1);
  }

  if (cc.secondary_pci_native)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Secondary device is PCI Native\n");
    drives_by_index_num[2].base_cmd_regs_port = pci_read_raw_reg(_address, PCI_REGS::BAR_2) & 0xFFF0;
    drives_by_index_num[3].base_cmd_regs_port = drives_by_index_num[2].base_cmd_regs_port;

    drives_by_index_num[2].base_control_port = pci_read_raw_reg(_address, PCI_REGS::BAR_3) & 0xFFF0;
    drives_by_index_num[3].base_control_port = drives_by_index_num[2].base_control_port;

    channel_irq_nums[1] = compute_irq_for_pin(interrupt_reg.interrupt_pin - 1);
  }

  bus_master_base_port = pci_read_raw_reg(_address, PCI_REGS::BAR_4) & 0xFFF0;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Bus master base port: ", bus_master_base_port, "\n");

  KL_TRC_EXIT;
}

bool pci_controller::issue_command(uint16_t drive_index,
                                   COMMANDS command,
                                   uint16_t features,
                                   uint16_t count,
                                   uint64_t lba_addr,
                                   void *buffer,
                                   uint64_t buffer_len)
{
  bool result{true};
  uint8_t drive_select{0};
  uint8_t ident_output;

  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Execute ATA command: ", static_cast<uint64_t>(command), " on drive: ", drive_index, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "- Address: ", lba_addr, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "- Features: ", features, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "- Count: ", count, "\n");

  ASSERT(static_cast<uint16_t>(command) < num_known_commands);
  ASSERT(drive_index < MAX_DRIVE_IDX);
  const command_properties &command_props = known_commands[static_cast<uint16_t>(command)];
  const drive_details &drive = drives_by_index_num[drive_index];

  if (drive_index >= MAX_DRIVE_IDX)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Request for an invalid drive\n");
    result = false;
  }

  if ((command_props.max_sectors != -1) &&
      (((command_props.max_sectors == 0) && (count != 0)) ||
       (count > command_props.max_sectors)))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Transfer of too many sectors requested\n");
    result = false;
  }

  if (command_props.dma_command)
  {
    bm_ide_status st_byte;
    KL_TRC_TRACE(TRC_LVL::FLOW, "Make sure we have DMA mutex\n");
    result = continue_with_dma_setup();

    st_byte.raw = read_bus_master_reg(BUS_MASTER_PORTS::STATUS, drive.channel_number);
    st_byte.interrupt_status = 0;
    st_byte.dma_error = 0;
    st_byte.bus_master_active = 0;
    write_bus_master_reg(BUS_MASTER_PORTS::STATUS, drive.channel_number, st_byte.raw);
  }

  if (result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt to continue\n");

    drive_select = drive.drive_select_byte;
    if (command_props.lba_command)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Add LBA flag to drive select\n");
      drive_select |= 0x40;
    }

    klib_synch_spinlock_lock(cmd_spinlock);
    interrupt_on_chan[drive.channel_number] = false;

    write_ata_cmd_port(drive_index, DRIVE_SELECT_PORT, drive_select);
    if (command_props.lba48_command)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Send long LBA address part\n");
      write_ata_cmd_port(drive_index, NUM_SECTORS_PORT, ((count & 0xFF00) >> 8)); // Sector count high byte
      write_ata_cmd_port(drive_index, LBA_LOW_BYTE, ((lba_addr & 0x0000FF000000) >> 24)); // LBA byte 4
      write_ata_cmd_port(drive_index, LBA_MID_BYTE, ((lba_addr & 0x00FF00000000) >> 32)); // LBA byte 5
      write_ata_cmd_port(drive_index, LBA_HIGH_BYTE, ((lba_addr & 0xFF0000000000) >> 40)); // LBA byte 6
    }
    write_ata_cmd_port(drive_index, NUM_SECTORS_PORT, (count & 0xFF)); // Sector count low byte
    write_ata_cmd_port(drive_index, LBA_LOW_BYTE, (lba_addr & 0x0000000000FF)); // LBA byte 1
    write_ata_cmd_port(drive_index, LBA_MID_BYTE, ((lba_addr & 0x00000000FF00) >> 8)); // LBA byte 2
    write_ata_cmd_port(drive_index, LBA_HIGH_BYTE, ((lba_addr & 0x000000FF0000) >> 16)); // LBA byte 3
    write_ata_cmd_port(drive_index, COMMAND_STATUS_PORT, known_commands[static_cast<uint16_t>(command)].command_code);

    // The identify command writes a byte immediately if the drive doesn't exist.
    if (command == COMMANDS::IDENTIFY)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "IDENTIFY command quick response check\n");
      ident_output = read_ata_cmd_port(drive_index, COMMAND_STATUS_PORT);
      if (ident_output == 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "No device attached\n");
        result = false;
      }

      // We don't get an interrupt when this command completes, for some reason, so fake it.
      interrupt_on_chan[drive.channel_number] = true;
    }

    if (result && command_props.dma_command)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Start bus mastering\n");
      start_bus_master(drive.channel_number);
    }

    if (result)
    {
      result = wait_for_cmd_completion(drive_index);
      KL_TRC_TRACE(TRC_LVL::FLOW, "Polling wait result: ", result, "\n");
    }

    if (result && command_props.dma_command)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "DMA command, await completion\n");
      KL_TRC_TRACE(TRC_LVL::FLOW, "Bus master status: ", read_bus_master_reg(STATUS, drive.channel_number), "\n");
      stop_bus_master(drive.channel_number);
    }

    if (result && command_props.reads_sectors)
    {
      // Read commands will have left data somewhere that needs to be copied to the requested target buffer.
      if (dma_transfer_is_read)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "DMA read command, reading output\n");
        dma_read_sectors_to_buffers();
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Non-DMA read command, reading output\n");
        pio_read_sectors_to_buffer(drive_index, count, reinterpret_cast<unsigned char *>(buffer), buffer_len);
      }
    }
    else if (result && command_props.writes_sectors && !command_props.dma_command)
    {
      // Do the write.
      result = pio_write_sectors_to_drive(drive_index, count, reinterpret_cast<uint8_t *>(buffer), buffer_len);
    } // There's no DMA equivalent because the DMA has already done the copying.

    if (command_props.dma_command)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Release DMA mutex\n");
      klib_synch_mutex_release(dma_mutex, false);
    }

    klib_synch_spinlock_unlock(cmd_spinlock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool pci_controller::dma_transfer_supported()
{
  return this->bm_enabled();
}

/// @brief Write a value to an ATA port for the specified drive.
///
/// @param drive_index The index of the device to command.
///
/// @param port The port of that drive to write to.
///
/// @param value The value to write out.
void pci_controller::write_ata_cmd_port(uint16_t drive_index, ATA_PORTS port, uint8_t value)
{
  KL_TRC_ENTRY;

  ASSERT(port != ATA_PORTS::DATA_PORT);
  ASSERT(drive_index < MAX_DRIVE_IDX);

  proc_write_port(drives_by_index_num[drive_index].base_cmd_regs_port + port, value, 8);

  KL_TRC_EXIT;
}

/// @brief Read a value from an ATA port on the specified drive.
///
/// @param drive_index The index of the device to read from.
///
/// @param port The port to read from
///
/// @return The value returned by the device.
uint8_t pci_controller::read_ata_cmd_port(uint16_t drive_index, ATA_PORTS port)
{
  uint8_t res;

  KL_TRC_ENTRY;
  ASSERT(port != ATA_PORTS::DATA_PORT);
  ASSERT(drive_index < MAX_DRIVE_IDX);

  res = static_cast<uint8_t>(proc_read_port(drives_by_index_num[drive_index].base_cmd_regs_port + port, 8));

  KL_TRC_EXIT;
  return res;
}

/// @brief Wait until the most recently issued command has been completed.
///
/// This means waiting for the device to interrupt that it is no longer busy and then confirming it using the status
/// register.
///
/// @param drive_index The drive to wait for.
///
/// @return True if the device completed the command successfully, false otherwise.
bool pci_controller::wait_for_cmd_completion(uint16_t drive_index)
{
  bool result = true;
  uint8_t channel;

  KL_TRC_ENTRY;

  ASSERT(drive_index < MAX_DRIVE_IDX);
  channel = drives_by_index_num[drive_index].channel_number;

  while (!interrupt_on_chan[channel]) { };
  interrupt_on_chan[channel] = false;

  result = poll_wait_for_drive_not_busy(drive_index);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Wait for the drive to declare that it is not busy by polling its status register.
///
/// @param drive_index The drive to wait on.
///
/// @return True if the device is not busy and not errored. False if an error occurs.
bool pci_controller::poll_wait_for_drive_not_busy(uint16_t drive_index)
{
  KL_TRC_ENTRY;

  status_byte result;
  uint8_t result_c;
  bool ret;

  // Do 4 dummy reads to flush the status
  read_ata_cmd_port(drive_index, COMMAND_STATUS_PORT);
  read_ata_cmd_port(drive_index, COMMAND_STATUS_PORT);
  read_ata_cmd_port(drive_index, COMMAND_STATUS_PORT);
  read_ata_cmd_port(drive_index, COMMAND_STATUS_PORT);

  do
  {
    result_c = read_ata_cmd_port(drive_index, COMMAND_STATUS_PORT);
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
    KL_TRC_TRACE(TRC_LVL::FLOW, "Success with byte: ", result_c, "\n");
    ret = true;
  }

  KL_TRC_EXIT;

  return ret;
}

/// @brief Do a PIO read from a device to a buffer
///
/// @param drive_index Which drive are we addressing?
///
/// @param sectors How many sectors to read
///
/// @param[out] buffer The buffer to write in to.
///
/// @param buffer_length The number of bytes in buffer.
void pci_controller::pio_read_sectors_to_buffer(uint16_t drive_index,
                                                uint16_t sectors,
                                                unsigned char *buffer,
                                                uint64_t buffer_length)
{
  KL_TRC_ENTRY;

  uint16_t *int_buffer = reinterpret_cast<uint16_t *>(buffer);

  ASSERT(buffer_length >= SECTOR_LENGTH * sectors);
  ASSERT(drive_index < MAX_DRIVE_IDX);

  for (uint64_t i = 0; i < (SECTOR_LENGTH * sectors); i += 2)
  {
    if ((i != 0) && (i % (SECTOR_LENGTH / 2) == 0))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Time for a pause\n");
      time_stall_process(400);
      if (!poll_wait_for_drive_not_busy(drive_index))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Waiting failed\n");
        break;
      }
    }
    int_buffer[i / 2] = proc_read_port(drives_by_index_num[drive_index].base_cmd_regs_port + DATA_PORT, 16);
  }

  KL_TRC_EXIT;
}

/// @brief Copy our bounce buffers to the user-provided buffer after completion of a DMA transfer.
void pci_controller::dma_read_sectors_to_buffers()
{
  uint8_t *bounce_buffer;
  uint32_t real_byte_length;

  KL_TRC_ENTRY;

  bounce_buffer = reinterpret_cast<uint8_t *>(prd_table);

  bounce_buffer += 65536;
  for (uint16_t i = 0; i < num_prd_table_entries; i++)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Copy block index: ", i);
    real_byte_length = transfer_block_details[i].bytes_to_transfer;
    if (real_byte_length == 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Copy 64kB\n"); // This behaviour is specified in the ATA spec.
      real_byte_length = 65536;
    }
    KL_TRC_TRACE(TRC_LVL::FLOW, ", length: ", real_byte_length,
                                " from: ", bounce_buffer,
                                " to: ", transfer_block_details[i].buffer, "\n");
    kl_memcpy(bounce_buffer, transfer_block_details[i].buffer, real_byte_length);
    bounce_buffer += real_byte_length; ///< TODO: Is this correct??
  }

  KL_TRC_EXIT;
}

bool pci_controller::handle_translated_interrupt_fast(unsigned char interrupt_offset,
                                                      unsigned char raw_interrupt_num) // Actually the IRQ number.
{
  uint16_t port_num = static_cast<uint16_t>(BUS_MASTER_PORTS::STATUS) + bus_master_base_port;
  bm_ide_status status;
  bool was_for_us = false;

  KL_TRC_ENTRY;

  // Reset the interrupt pending flag. We're dealing with it! Bear in mind that both IRQ numbers could be the same, so
  // we do the check for both controller channels.
  if (raw_interrupt_num == channel_irq_nums[0])
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Primary interrupt\n");
    status.raw = proc_read_port(port_num, 8);

    if (status.dma_error)
    {
      KL_TRC_TRACE(TRC_LVL::IMPORTANT, "DMA error result\n");
    }
    if (status.bus_master_active)
    {
      KL_TRC_TRACE(TRC_LVL::IMPORTANT, "DMA active\n");
    }

    if (status.interrupt_status == 1)
    {
      status.raw = 0;
      status.interrupt_status = 1; // Set 1 to clear.
      proc_write_port(port_num, status.raw, 8);
      was_for_us = true;
    }
  }

  if ((!was_for_us) && (raw_interrupt_num == channel_irq_nums[1]))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Secondary interrupt\n");
    port_num += 8;
    status.raw = proc_read_port(port_num, 8);
    if (status.interrupt_status == 1)
    {
      status.raw = 0;
      status.interrupt_status = 1; // Set 1 to clear.
      proc_write_port(port_num, status.raw, 8);
      was_for_us = true;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Needs handling? ", was_for_us, "\n");
  KL_TRC_EXIT;

  return was_for_us; // For now, always allow interrupts to be handled in the slow path.
}

void pci_controller::handle_translated_interrupt_slow(unsigned char interrupt_offset,
                                                      unsigned char raw_interrupt_num)
{
  KL_TRC_ENTRY;

  if (raw_interrupt_num == channel_irq_nums[0])
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Primary interrupt\n");
    interrupt_on_chan[0] = true;
  }
  else if (raw_interrupt_num == channel_irq_nums[1])
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Secondary interrupt\n");
    interrupt_on_chan[1] = true;
  }

  KL_TRC_EXIT;
}

bool pci_controller::start_prepare_dma_transfer(bool is_read, uint16_t drive_index)
{
  bool result = true;
  SYNC_ACQ_RESULT mr;

  KL_TRC_ENTRY;

  ASSERT(drive_index < MAX_DRIVE_IDX);

  // If acquired, this mutex is not released at the end of this function, because the owning thread needs to use it
  // when it calls queue_dma_transfer().
  mr = klib_synch_mutex_acquire(dma_mutex, MUTEX_MAX_WAIT);

  // Clear the interrupt flag.
  write_bus_master_reg(STATUS, drives_by_index_num[drive_index].channel_number, 0x04);

  if (mr == SYNC_ACQ_ACQUIRED)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Acquired mutex\n");

    if (buffer_phys_addr == 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Initialise DMA transfer buffers\n");
      ASSERT(buffer_virt_addr == 0);

      prd_table = reinterpret_cast<prd_table_entry *>(kmalloc(MEM_PAGE_SIZE));
      buffer_virt_addr = reinterpret_cast<uint64_t>(prd_table);
      buffer_phys_addr = reinterpret_cast<uint64_t>(mem_get_phys_addr(prd_table));
      ASSERT((buffer_phys_addr & 0xFFFFFFFF00000000) == 0);

    }
    dma_transfer_drive_idx = drive_index;
    num_prd_table_entries = 0;
    prd_table[0].end_of_table = 1;
    dma_transfer_is_read = is_read;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to acquire DMA mutex\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool pci_controller::queue_dma_transfer_block(void *buffer, uint16_t bytes_this_block)
{
  bool result = true;
  uint64_t entry_phys_ptr;
  uint64_t entry_virt_ptr;
  uint32_t actual_num_bytes;

  KL_TRC_ENTRY;

  result = continue_with_dma_setup();

  if (result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can continue setup\n");

    if (num_prd_table_entries >= max_prd_table_entries)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Too many items queued\n");
      result = false;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Current num PRD table entries: ", num_prd_table_entries, "\n");
      entry_phys_ptr = buffer_phys_addr + (65536 * (num_prd_table_entries + 1));
      ASSERT((entry_phys_ptr & 0xFFFFFFFF00000000) == 0);
      entry_virt_ptr = buffer_virt_addr + (65536 * (num_prd_table_entries + 1));

      prd_table[num_prd_table_entries].region_phys_base_addr = static_cast<uint32_t>(entry_phys_ptr);
      KL_TRC_TRACE(TRC_LVL::FLOW, "Queue new transfer item - ptr: ",
                                  prd_table[num_prd_table_entries].region_phys_base_addr, "\n");
      prd_table[num_prd_table_entries].end_of_table = true;
      prd_table[num_prd_table_entries].byte_count = bytes_this_block;

      transfer_block_details[num_prd_table_entries].buffer = buffer;
      transfer_block_details[num_prd_table_entries].bytes_to_transfer = bytes_this_block;

      if (!dma_transfer_is_read)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Transfer is write-to-drive, copy to bounce buffer\n");
        actual_num_bytes = bytes_this_block;
        if (actual_num_bytes == 0) // This is specified in the ATA Host Controller spec.
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Copy 64k\n");
          actual_num_bytes = 65536;
        }

        kl_memcpy(buffer, reinterpret_cast<void *>(entry_virt_ptr), actual_num_bytes);
      }

      if (num_prd_table_entries > 0)
      {
        prd_table[num_prd_table_entries - 1].end_of_table = false;
      }
      num_prd_table_entries++;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Write the PRD table address to the Bus Master controller for the given channel.
///
/// @param address The physical address of the start of the PRD table. Note that this is a 32-bit address!
///
/// @param channel The channel to write this the PRD address to.
void pci_controller::write_prd_table_addr(uint32_t address, uint16_t channel)
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Write address ", address, " to channel ", channel, "\n");
  uint16_t port_num = static_cast<uint16_t>(BUS_MASTER_PORTS::PRD_TABLE_ADDR_0) + bus_master_base_port;
  port_num += (channel * 8);

  proc_write_port(port_num, address, 32);

  KL_TRC_EXIT;
}

/// @brief Set the direction of the next bus master transfer.
///
/// @param is_read True if this is a read from disk to RAM, false if it is a write from RAM to disk.
///
/// @param channel The channel to make this change for.
void pci_controller::set_bus_master_direction(bool is_read, uint16_t channel)
{
  bus_master_cmd_byte command_byte;
  KL_TRC_ENTRY;

  command_byte.raw = read_bus_master_reg(BUS_MASTER_PORTS::COMMAND, channel);
  command_byte.is_write = !is_read;
  write_bus_master_reg(BUS_MASTER_PORTS::COMMAND, channel, command_byte.raw);

  KL_TRC_EXIT;
}

/// @brief Set the bus master operation flag.
///
/// @param channel The channel to set the flag on.
void pci_controller::start_bus_master(uint16_t channel)
{
  bus_master_cmd_byte command_byte;
  KL_TRC_ENTRY;

  command_byte.raw = read_bus_master_reg(BUS_MASTER_PORTS::COMMAND, channel);
  command_byte.start = 1;
  write_bus_master_reg(BUS_MASTER_PORTS::COMMAND, channel, command_byte.raw);

  KL_TRC_EXIT;
}

/// @brief Clear the bus master operation flag.
///
/// @param channel The channel to set the flag on.
void pci_controller::stop_bus_master(uint16_t channel)
{
  bus_master_cmd_byte command_byte;
  KL_TRC_ENTRY;

  command_byte.raw = read_bus_master_reg(BUS_MASTER_PORTS::COMMAND, channel);
  command_byte.start = 0;
  write_bus_master_reg(BUS_MASTER_PORTS::COMMAND, channel, command_byte.raw);

  KL_TRC_EXIT;
}

/// @brief Write a bus master register for the specified channel.
///
/// @param port The register/port to write.
///
/// @param channel Which channel to execute the write on
///
/// @param val The value to write.
void pci_controller::write_bus_master_reg(BUS_MASTER_PORTS port, uint16_t channel, uint8_t val)
{
  uint16_t port_num;
  KL_TRC_ENTRY;

  port_num = static_cast<uint16_t>(port) + bus_master_base_port;
  port_num += (channel * 8);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Write ", val, " to port ", port_num, "\n");

  proc_write_port(port_num, val, 8);

  KL_TRC_EXIT;
}

/// @brief Read a bus master register for the specified channel.
///
/// @param port The port/register to read.
///
/// @param channel The channel to do the read on.
///
/// @return The value returned.
uint8_t pci_controller::read_bus_master_reg(BUS_MASTER_PORTS port, uint16_t channel)
{
  uint8_t result;
  uint16_t port_num;

  KL_TRC_ENTRY;

  port_num = static_cast<uint16_t>(port) + bus_master_base_port;
  port_num += (channel * 8);

  result = proc_read_port(port_num, 8);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Confirm that this thread owns the mutex protecting DMA transfers.
///
/// @return true if this thread owns the DMA mutex, in which case it can continue setting up a DMA transfer, false
///         otherwise.
bool pci_controller::continue_with_dma_setup()
{
  bool result = true;
  SYNC_ACQ_RESULT mr;

  KL_TRC_ENTRY;

  mr = klib_synch_mutex_acquire(dma_mutex, 0);

  switch(mr)
  {
  case SYNC_ACQ_ACQUIRED:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Acquired mutex, setup not started yet\n");
    result = false;
    klib_synch_mutex_release(dma_mutex, false);
    break;

  case SYNC_ACQ_TIMEOUT:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex owned by another thread\n");
    result = false;
    break;

  case SYNC_ACQ_ALREADY_OWNED:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex owned by us already - can continue\n");
    break;

  default:
    panic("Unknown response to acquiring mutex");
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Using PIO mode, write data to a drive.
///
/// @param drive_index The index of the drive to write on.
///
/// @param sectors How many sectors to write on the drive.
///
/// @param buffer The buffer to write from.
///
/// @param buffer_length The number of bytes in the buffer.
///
/// @return True if the write succeeded, false if the drive failed.
bool pci_controller::pio_write_sectors_to_drive(uint16_t drive_index,
                                                uint16_t sectors,
                                                uint8_t *buffer,
                                                uint64_t buffer_length)
{
  bool result{true};
  uint16_t *buffer_16{reinterpret_cast<uint16_t *>(buffer)};

  KL_TRC_ENTRY;

  ASSERT(buffer_16 != nullptr);

  if (buffer_length < (sectors * SECTOR_LENGTH))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Insufficient data to write...\n");
    result = false;
  }
  else
  {
    for (uint16_t i = 0; i < sectors; i++)
    {
      for (uint16_t j = 0; j < (SECTOR_LENGTH / 2); j++, buffer_16++)
      {
        proc_write_port(drives_by_index_num[drive_index].base_cmd_regs_port + DATA_PORT, *buffer_16, 16);
      }
      if (!poll_wait_for_drive_not_busy(drive_index))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Drive failed after ", i, " sectors\n");
        result = false;
        break;
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool pci_controller::dma_transfer_blocks_queued()
{
  bool result;

  KL_TRC_ENTRY;

  result = continue_with_dma_setup();

  if (result)
  {
    for (uint16_t i = 0; i < MAX_CHANNEL; i++)
    {
      write_prd_table_addr(buffer_phys_addr, i);
    }

    set_bus_master_direction(dma_transfer_is_read, drives_by_index_num[dma_transfer_drive_idx].channel_number);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
