/// @file
/// @brief Implements a basic serial port driver.
//
// Known defects:
// - Doesn't attempt to match COM1 driver to COM1 on the board - in the kernel COM1 is simply the first COM port it
//   finds out about.

//#define ENABLE_TRACING

#include "serial.h"
#include "klib/klib.h"
#include "acpi/acpi_if.h"
#include "processor/processor.h"

/// @brief Construct a new serial port object
///
/// @param obj_handle ACPI handle for this object.
serial_port::serial_port(ACPI_HANDLE obj_handle) :
  IDevice{"Serial port", "COM", true}
{
ACPI_STATUS status;
  ACPI_BUFFER buf;
  uint8_t *raw_ptr;
  ACPI_RESOURCE *resource_ptr;

  KL_TRC_ENTRY;

  buf.Length = ACPI_ALLOCATE_BUFFER;
  buf.Pointer = nullptr;

  set_device_status(DEV_STATUS::STOPPED);

  // Iterate over all provided resources to find one which tells us the CMOS port. Probably it's 0x70...
  status = AcpiGetCurrentResources(obj_handle, &buf);
  if (status == AE_OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully got current resources\n");
    raw_ptr = reinterpret_cast<uint8_t *>(buf.Pointer);
    resource_ptr = reinterpret_cast<ACPI_RESOURCE *>(raw_ptr);

    while ((resource_ptr->Length != 0) && (resource_ptr->Type != ACPI_RESOURCE_TYPE_END_TAG))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Resource type: ", resource_ptr->Type, "\n");
      switch (resource_ptr->Type)
      {
      case ACPI_RESOURCE_TYPE_IO:
        KL_TRC_TRACE(TRC_LVL::FLOW, "IO resources\n");
        com_base_port = resource_ptr->Data.Io.Minimum;

        break;

      case ACPI_RESOURCE_TYPE_IRQ:
        KL_TRC_TRACE(TRC_LVL::FLOW, "IRQ list\n");
        if (resource_ptr->Data.Irq.InterruptCount != 1)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Too many interrupts - unable to handle\n");
          set_device_status(DEV_STATUS::FAILED);
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Handle IRQ: ", resource_ptr->Data.Irq.Interrupts[0], "\n");
          irq_number = resource_ptr->Data.Irq.Interrupts[0];
        }

        break;

      case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Extended IRQ list\n");
        if (resource_ptr->Data.ExtendedIrq.InterruptCount != 1)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Too many interrupts - unable to handle\n");
          set_device_status(DEV_STATUS::FAILED);
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Handle IRQ: ", resource_ptr->Data.ExtendedIrq.Interrupts[0], "\n");
          irq_number = resource_ptr->Data.ExtendedIrq.Interrupts[0];
        }
        break;
      }

      raw_ptr += resource_ptr->Length;
      resource_ptr = reinterpret_cast<ACPI_RESOURCE *>(raw_ptr);
    }

    proc_register_irq_handler(irq_number, this);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to get resources\n");
    set_device_status(DEV_STATUS::FAILED);
  }

  if (buf.Pointer != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Release buffer\n");
    AcpiOsFree(buf.Pointer);
  }

  // Construct a pipe for storing data transmitted to the UART ready for reading by other kernel objects.
  input_branch = pipe_branch::create();
  std::shared_ptr<ISystemTreeLeaf> leaf;
  ASSERT(input_branch->get_child("read", leaf) == ERR_CODE::NO_ERROR);
  pipe_read_leaf = std::dynamic_pointer_cast<pipe_branch::pipe_read_leaf>(leaf);
  ASSERT(pipe_read_leaf);

  ASSERT(input_branch->get_child("write", leaf) == ERR_CODE::NO_ERROR);
  pipe_write_leaf = std::dynamic_pointer_cast<pipe_branch::pipe_write_leaf>(leaf);
  ASSERT(pipe_write_leaf);

  // Don't block. This is an arbitrary decision that should be controllable by the user of this port.
  pipe_read_leaf->set_block_on_read(false);

  KL_TRC_EXIT;
}

serial_port::~serial_port()
{
  KL_TRC_ENTRY;

  if (irq_number != 0xFFFF)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unregister IRQ handler\n");
    proc_unregister_irq_handler(irq_number, this);
  }

  KL_TRC_EXIT;
}

/// @brief Set an object to receive messages when new data arrives at this serial port.
///
/// @param new_handler The object to receive messages.
void serial_port::set_msg_receiver(std::shared_ptr<work::message_receiver> &new_handler)
{
  KL_TRC_ENTRY;

  new_data_handler = new_handler;

  KL_TRC_EXIT;
}

bool serial_port::start()
{
  KL_TRC_ENTRY;

  if (get_device_status() != DEV_STATUS::FAILED)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Startup\n");

    set_device_status(DEV_STATUS::STARTING);

    proc_write_port(com_base_port + 1, 0x00, 8); // Disable all interrupts
    proc_write_port(com_base_port + 3, 0x80, 8); // Enable DLAB (set baud rate divisor)
    proc_write_port(com_base_port + 0, 0x03, 8); // Set divisor to 3 (lo byte) 38400 baud
    proc_write_port(com_base_port + 1, 0x00, 8); //                  (hi byte)
    proc_write_port(com_base_port + 3, 0x03, 8); // 8 bits, no parity, one stop bit
    proc_write_port(com_base_port + 2, 0xC7, 8); // Enable FIFO, clear them, with 14-byte threshold
    proc_write_port(com_base_port + 4, 0x0B, 8); // IRQs enabled, RTS/DSR set
    proc_write_port(com_base_port + 1, 0x01, 8); // Enable receiver interrupt

    set_device_status(DEV_STATUS::OK);
  }

  KL_TRC_EXIT;
  return true;
}

bool serial_port::stop()
{
  KL_TRC_ENTRY;

  if (get_device_status() != DEV_STATUS::FAILED)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Shutdown\n");

    set_device_status(DEV_STATUS::STOPPING);

    proc_write_port(com_base_port + 1, 0x00, 8); // Disable all interrupts

    set_device_status(DEV_STATUS::STOPPED);
  }

  KL_TRC_EXIT;
  return true;
}

bool serial_port::reset()
{
  KL_TRC_ENTRY;

  if (get_device_status() != DEV_STATUS::FAILED)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Reset\n");

    set_device_status(DEV_STATUS::RESET);

    proc_write_port(com_base_port + 1, 0x00, 8); // Disable all interrupts
    proc_write_port(com_base_port + 3, 0x80, 8); // Enable DLAB (set baud rate divisor)
    proc_write_port(com_base_port + 0, 0x03, 8); // Set divisor to 3 (lo byte) 38400 baud
    proc_write_port(com_base_port + 1, 0x00, 8); //                  (hi byte)
    proc_write_port(com_base_port + 3, 0x03, 8); // 8 bits, no parity, one stop bit
    proc_write_port(com_base_port + 2, 0xC7, 8); // Enable FIFO, clear them, with 14-byte threshold

    set_device_status(DEV_STATUS::STOPPED);
  }

  KL_TRC_EXIT;
  return true;
}

bool serial_port::handle_interrupt_fast(uint8_t interrupt_number)
{
  // Always move to the slow path
  return true;
}

void serial_port::handle_interrupt_slow(uint8_t interrupt_number)
{
  uint8_t c;
  uint64_t bytes_written{0};
  ERR_CODE ec;
  bool new_bytes{false};

  KL_TRC_ENTRY;

  while ((proc_read_port(com_base_port + 5, 8) & 1) == 1)
  {
    new_bytes = true;
    c = static_cast<uint8_t>(proc_read_port(com_base_port, 8));
    KL_TRC_TRACE(TRC_LVL::FLOW, "char: ", c, "\n");
    ec = pipe_write_leaf->write_bytes(0, 1, &c, 1, bytes_written);
    if ((bytes_written != 1) || (ec != ERR_CODE::NO_ERROR))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Pipe write failed\n");
      set_device_status(DEV_STATUS::FAILED);
    }
  }

  // If we managed to read some bytes from the serial port then signal any interested party. We do this here rather
  // than relying on the pipe's built in mechanism for this to avoid sending one message per byte!
  if (new_bytes)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Some bytes received, maybe send new data message\n");
    std::shared_ptr<work::message_receiver> rcv = new_data_handler.lock();
    if (rcv)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Send new data message\n");
      std::unique_ptr<msg::root_msg> msg = std::make_unique<msg::root_msg>(SM_PIPE_NEW_DATA);
      work::queue_message(rcv, std::move(msg));
    }
  }

  KL_TRC_EXIT;
}

ERR_CODE serial_port::read_bytes(uint64_t start,
                                 uint64_t length,
                                 uint8_t *buffer,
                                 uint64_t buffer_length,
                                 uint64_t &bytes_read)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

  if (get_device_status() == DEV_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt read\n");
    result = pipe_read_leaf->read_bytes(start, length, buffer, buffer_length, bytes_read);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Port not running\n");
    result = ERR_CODE::DEVICE_FAILED;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

  // Overrides of IWritable
ERR_CODE serial_port::write_bytes(uint64_t start,
                                  uint64_t length,
                                  const uint8_t *buffer,
                                  uint64_t buffer_length,
                                  uint64_t &bytes_written)
{
  uint64_t actual_length{std::min(length, buffer_length)};

  KL_TRC_ENTRY;

  for (uint64_t i = 0; i < actual_length; i++)
  {
    while (!(bool(proc_read_port(com_base_port + 5, 8) & 0x20)))
    {
      //spin!
    }
    proc_write_port(com_base_port, static_cast<uint64_t>(buffer[i]), 8);
  }

  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}
