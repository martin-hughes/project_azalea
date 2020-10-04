/// @file
/// @brief Implementation of a pseudo-filesystem for accessing device driver objects.
///
/// The 'filesystem' will (eventually) keep track of all active devices in the system. The actual device drivers are
/// in the "devices" tree.

//#define ENABLE_TRACING

#include "kernel_all.h"
#include "types/block_wrapper.h"

#include "dev_fs.h"

#include "../devices/pci/pci.h"
#include "../devices/legacy/ps2/ps2_controller.h"
#include "../devices/legacy/serial/serial.h"
#include "../devices/terminals/vga_terminal.h"
#include "../devices/terminals/serial_terminal.h"

// TEMP Assistance constructing a filesystem until the device monitor is more developed.
#include "../system_tree/fs/fat/fat_fs.h"
#include "system_tree.h"
#include "../devices/block/proxy/block_proxy.h"
#include "../devices/block/ata/ata_device.h"
#include "../devices/virtio/virtio_block.h"
std::shared_ptr<fat_filesystem> setup_initial_fs(std::shared_ptr<IBlockDevice> first_hdd);

/// @cond
// Temporary variables
extern generic_keyboard *keyb_ptr;
extern std::shared_ptr<terms::generic> *term_ptr;
generic_keyboard *keyb_ptr;
std::shared_ptr<terms::generic> *term_ptr{nullptr};
/// @endcond

/// @brief Simple constructor
dev_root_branch::dev_root_branch()
{
  KL_TRC_ENTRY;

  dev_slash_null = std::make_shared<null_file>();

  ASSERT(this->add_child("null", dev_slash_null) == ERR_CODE::NO_ERROR);

  KL_TRC_EXIT;
}

dev_root_branch::~dev_root_branch()
{
  INCOMPLETE_CODE("~dev_root_branch()");
}

/// @brief Scan the system for hardware
///
/// At the moment this isn't very advanced, but in future it should look for all possible devices in the system. It may
/// also need splitting out into a constantly running process to deal with plug-n-play.
void dev_root_branch::scan_for_devices()
{
  unsigned char *display_ptr;
  std::shared_ptr<IDevice> empty;

  KL_TRC_ENTRY;

  dev::monitor::init();

  // Scan the ACPI namespace for any devices.
  acpi_create_devices();

  // Add a PCI root device. This will scan for its own devices automatically.
  std::shared_ptr<pci_root_device> pci_root;
  ASSERT(dev::create_new_device(pci_root, empty));
  this->add_child("pci", pci_root);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // TEMP                                                                                                     //
  // Below here is all shortcuts to construct some extra devices before the device monitor is more developed. //
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // Temporarily assume the presence of a PS/2 controller.
  std::shared_ptr<gen_ps2_controller_device> ps2;
  ASSERT(dev::create_new_device(ps2, empty));

  // Temporarily assume the presence of a VGA card on which to run a text terminal.
  display_ptr = reinterpret_cast<unsigned char *>(mem_allocate_virtual_range(1));
  mem_map_range(nullptr, display_ptr, 1);
  display_ptr += 0xB8000;
  std::shared_ptr<IWritable> fake_stream;

  std::shared_ptr<terms::vga> term;
  ASSERT(dev::create_new_device(term, empty, fake_stream, display_ptr));

  // See if the PS/2 controller has a keyboard attached, and if so get it to send its messages to the terminal.
  // Wait for the PS2 controller to be started first.
  while (ps2->get_device_status() != OPER_STATUS::OK)
  {

  }

  std::shared_ptr<generic_keyboard> keyb;
  if (ps2->_chan_1_dev_type == PS2_DEV_TYPE::KEYBOARD_MF2)
  {
    keyb = std::dynamic_pointer_cast<generic_keyboard>(ps2->chan_1_dev);
    ASSERT(keyb);
  }
  else if (ps2->_chan_2_dev_type == PS2_DEV_TYPE::KEYBOARD_MF2)
  {
    keyb = std::dynamic_pointer_cast<generic_keyboard>(ps2->chan_2_dev);
    ASSERT(keyb);
  }
  else
  {
    panic("No keyboard!");
  }
  std::shared_ptr<work::message_receiver> t_r = std::dynamic_pointer_cast<work::message_receiver>(term);
  ASSERT(t_r);
  keyb->set_receiver(t_r);

  // Setup a basic file system. Start by waiting for the first HDD to become ready.
  uint64_t start_time = time_get_system_timer_count(true);
  uint64_t end_time = start_time + (10ULL * 1000 * 1000 * 1000); // i.e. max wait of 10 seconds.

  std::shared_ptr<IHandledObject> hdd_leaf;
  std::shared_ptr<ata::generic_device> hdd_dev;
  bool ready{false};

  // Keep trying to get the first ATA device until it is ready or we run out of time.
  // There's an obvious assumption here that ATA1 is the desired HDD...
  while (time_get_system_timer_count(true) < end_time)
  {
    if (system_tree()->get_child("\\dev\\all\\ata1", hdd_leaf) == ERR_CODE::NO_ERROR)
    {
      kl_trc_trace(TRC_LVL::FLOW, "Got device leaf\n");
      hdd_dev = std::dynamic_pointer_cast<ata::generic_device>(hdd_leaf);
      if (hdd_dev)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Got device object\n");
        while ((hdd_dev->get_device_status() != OPER_STATUS::OK) &&
               (time_get_system_timer_count(true) < end_time))
        {
          // Just spin
        }

        if (hdd_dev->get_device_status() == OPER_STATUS::OK)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Started OK\n");
          ready = true;
          break;
        }
      }
    }
  }

  ASSERT(ready);

#if 0
  std::shared_ptr<virtio::block_device> virt_dev;
  start_time = time_get_system_timer_count(true);
  end_time = start_time + (10ULL * 1000 * 1000 * 1000); // i.e. max wait of 10 seconds.
  while(1)
  {
    if (system_tree()->get_child("\\dev\\all\\virtio-blk1", hdd_leaf) == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Got device leaf\n");
      virt_dev = std::dynamic_pointer_cast<virtio::block_device> (hdd_leaf);
      if (virt_dev)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Got device object\n");
        while ((virt_dev->get_device_status() != OPER_STATUS::OK) &&
               (time_get_system_timer_count(true) < end_time))
        {
          // Just spin
        }

        if (virt_dev->get_device_status() == OPER_STATUS::OK)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Started virtdev OK\n");
          char buf[512];
          virt_dev->read_blocks(0, 1, buf, 512);
          break;
        }
      }
    }
  }
#endif

  std::shared_ptr<fat_filesystem> first_fs = setup_initial_fs(hdd_dev);
  ASSERT(first_fs != nullptr);
  ASSERT(system_tree()->add_child("\\root", std::dynamic_pointer_cast<ISystemTreeBranch>(first_fs)) == ERR_CODE::NO_ERROR);

  keyb_ptr = keyb.get();
  term_ptr = new std::shared_ptr<terms::generic>();
  *term_ptr = term;


#ifdef INC_SERIAL_TERM
  std::shared_ptr<IHandledObject> leaf;
  ASSERT(system_tree()->get_child("\\dev\\all\\COM2", leaf) == ERR_CODE::NO_ERROR);
  std::shared_ptr<serial_port> port = std::dynamic_pointer_cast<serial_port>(leaf);
  ASSERT(port);

  std::shared_ptr<terms::serial> st;
  ASSERT(dev::create_new_device(st,
         empty,
         fake_stream,
         std::dynamic_pointer_cast<IWritable>(port),
         std::dynamic_pointer_cast<IReadable>(port)));

  std::shared_ptr<work::message_receiver> mr = std::dynamic_pointer_cast<work::message_receiver>(st);
  port->set_msg_receiver(mr);
#endif

  KL_TRC_EXIT;
}

dev_root_branch::dev_sub_branch::dev_sub_branch()
{

}

dev_root_branch::dev_sub_branch::~dev_sub_branch()
{

}

/// @brief Configure the filesystem of the (presumed) boot device as part of System Tree.
///
/// This function is temporary.
///
/// @param first_hdd Pointer to the HDD with the filesystem to load.
///
/// @return Pointer toa FAT filesystem presumed to exist on the first attached HDD.
std::shared_ptr<fat_filesystem> setup_initial_fs(std::shared_ptr<IBlockDevice> first_hdd)
{
  KL_TRC_ENTRY;
  ASSERT(first_hdd.get() != nullptr); // new ata::generic_device(nullptr, 0);
  std::unique_ptr<unsigned char[]> sector_buffer(new unsigned char[512]);
  std::shared_ptr<IDevice> empty;

  memset(sector_buffer.get(), 0, 512);

  std::shared_ptr<BlockWrapper> wrapper = BlockWrapper::create(first_hdd);

  if (wrapper->read_blocks(0, 1, sector_buffer.get(), 512) != ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Disk read failed\n");
    panic("Disk read failed :(\n");
  }

  // Confirm that we've loaded a valid MBR
  KL_TRC_TRACE(TRC_LVL::EXTRA, (uint64_t)sector_buffer[510], " ", (uint64_t)sector_buffer[511], "\n");
  ASSERT((sector_buffer[510] == 0x55) && (sector_buffer[511] == 0xAA));

  uint32_t start_sector;
  uint32_t sector_count;

  // Parse the MBR to find the first partition.
  memcpy(&start_sector, sector_buffer.get() + 454, 4);
  memcpy(&sector_count, sector_buffer.get() + 458, 4);

  kl_trc_trace(TRC_LVL::EXTRA, "First partition: ", (uint64_t)start_sector, " -> +", (uint64_t)sector_count, "\n");
  std::shared_ptr<block_proxy_device> pd;
  ASSERT(dev::create_new_device(pd, empty, first_hdd, start_sector, sector_count));
  while(pd->get_device_status() != OPER_STATUS::OK)
  { };

  // Initialise the filesystem based on that information
  std::shared_ptr<fat_filesystem> first_fs = fat_filesystem::create(pd);

  KL_TRC_EXIT;
  return first_fs;
}
