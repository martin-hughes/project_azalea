/// @file
/// @brief Implementation of a pseudo-filesystem for accessing device driver objects.
///
/// The 'filesystem' will (eventually) keep track of all active devices in the system. The actual device drivers are
/// in the "devices" tree.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/fs/dev/dev_fs.h"
#include "devices/device_monitor.h"

#include "acpi/acpi_if.h"
#include "devices/pci/pci.h"
#include "devices/legacy/ps2/ps2_controller.h"
#include "devices/terminals/vga_terminal.h"

// TEMP Assistance constructing a filesystem until the device monitor is more developed.
#include "system_tree/fs/fat/fat_fs.h"
#include "system_tree/system_tree.h"
std::shared_ptr<fat_filesystem> setup_initial_fs();

// TEMP this is a shortcut to put the first HDD into until the device monitor and dev FS are more connected.
#include "devices/block/proxy/block_proxy.h"
#include "devices/block/ata/ata_device.h"
extern ata::generic_device *first_hdd;
ata::generic_device *first_hdd{nullptr};

extern generic_keyboard *keyb_ptr;
extern std::shared_ptr<terms::generic> *term_ptr;
generic_keyboard *keyb_ptr;
std::shared_ptr<terms::generic> *term_ptr;

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
  while (ps2->get_device_status() != DEV_STATUS::OK)
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
  // This does occasionally hit a race condition where first_hdd is actually set to the second ATA device.
  while (first_hdd == nullptr) { };
  while(first_hdd->get_device_status() != DEV_STATUS::OK) { };
  kl_trc_trace(TRC_LVL::FLOW, "First HDD created\n");
  std::shared_ptr<fat_filesystem> first_fs = setup_initial_fs();
  ASSERT(first_fs != nullptr);
  ASSERT(system_tree()->add_child("\\root", std::dynamic_pointer_cast<ISystemTreeBranch>(first_fs)) == ERR_CODE::NO_ERROR);

  keyb_ptr = keyb.get();
  term_ptr = new std::shared_ptr<terms::generic>();
  *term_ptr = term;

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
/// @return Pointer toa FAT filesystem presumed to exist on the first attached HDD.
std::shared_ptr<fat_filesystem> setup_initial_fs()
{
  KL_TRC_ENTRY;
  ASSERT(first_hdd != nullptr); // new ata::generic_device(nullptr, 0);
  std::unique_ptr<unsigned char[]> sector_buffer(new unsigned char[512]);
#warning Should sort true parent-child relationship.
  std::shared_ptr<IDevice> empty;

  kl_memset(sector_buffer.get(), 0, 512);
  if (first_hdd->read_blocks(0, 1, sector_buffer.get(), 512) != ERR_CODE::NO_ERROR)
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
  kl_memcpy(sector_buffer.get() + 454, &start_sector, 4);
  kl_memcpy(sector_buffer.get() + 458, &sector_count, 4);

  kl_trc_trace(TRC_LVL::EXTRA, "First partition: ", (uint64_t)start_sector, " -> +", (uint64_t)sector_count, "\n");
  std::shared_ptr<block_proxy_device> pd;
  ASSERT(dev::create_new_device(pd, empty, first_hdd, start_sector, sector_count));
  while(pd->get_device_status() != DEV_STATUS::OK)
  { };

  // Initialise the filesystem based on that information
  std::shared_ptr<fat_filesystem> first_fs = fat_filesystem::create(pd);

  KL_TRC_EXIT;
  return first_fs;
}
