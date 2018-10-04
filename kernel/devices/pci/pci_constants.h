/// @file
/// @brief Constants relevant to all PCI devices.

#pragma once

#include <stdint.h>

// Legacy PCI configuration mechanism ports.
const uint16_t PCI_ADDRESS_PORT = 0xCF8;
const uint16_t PCI_DATA_PORT = 0xCFC;

// Invalid PCI vendor ID
const uint16_t PCI_INVALID_VENDOR = 0xFFFF;

// PCI Configuration Space register numbers.
enum class PCI_REGS : uint8_t
{
  DEV_AND_VENDOR_ID = 0,
  STATUS_AND_COMMAND = 1,
  CC_SC_PROG_IF_AND_REV_ID = 2,
  BIST_HT_LT_AND_CACHE_SIZE = 3,
  BAR_0 = 4,
  BAR_1 = 5,
  BAR_2 = 6,
  BAR_3 = 7,
  BAR_4 = 8,
  BAR_5 = 9,
  CARDBUS_CIS_PTR = 10,
  SUBSYS_AND_VENDOR_ID = 11,
  EXPANSION_ROM_PTR = 12,
  CAP_PTR = 13,
  LATS_AND_INTERRUPTS = 15,
};

// This table derives from the information at https://wiki.osdev.org/Pci
namespace PCI_CLASS
{
  const uint8_t UNCLASSIFIED = 0;
  const uint8_t MASS_STORE_CONTR = 1;
  const uint8_t NETWORK_CONTR = 2;
  const uint8_t DISPLAY_CONTR = 3;
  const uint8_t MULTIMEDIA_CONTR = 4;
  const uint8_t MEMORY_CONTR = 5;
  const uint8_t BRIDGE = 6;
  const uint8_t SIMPLE_COMM_CONTR = 7;
  const uint8_t BASE_PERIPHERAL = 8;
  const uint8_t INPUT_CONTR = 9;
  const uint8_t DOCKING_STATION = 10;
  const uint8_t PROCESSOR = 11;
  const uint8_t SERIAL_BUS_CONTR = 12;
  const uint8_t WIRELESS_CONTR = 13;
  const uint8_t INTELLIGENT_CONTR = 14;
  const uint8_t SATCOM_CONTR = 15;
  const uint8_t ENCRYPTION_CONTR = 16;
  const uint8_t SIGNAL_PROC_CONTR = 17;
  const uint8_t PROC_ACCEL = 18;
}

// This table also derives from the information at https://wiki.osdev.org/Pci
namespace PCI_SUBCLASS
{
  // Generic
  const uint8_t SUBCLASS_OTHER = 0x80;

  // Unclassified.
  const uint8_t NON_VGA_DEV = 0;
  const uint8_t VGA_COMPAT_DEV = 1;

  // Mass storage controllers.
  const uint8_t SCSI_BUS_CONTR = 0;
  const uint8_t IDE_CONTR = 1;
  const uint8_t FLOPPY_CONTR = 2;
  const uint8_t IPI_BUS_CONTR = 3;
  const uint8_t RAID_CONTR = 4;
  const uint8_t ATA_CONTR = 5;
  const uint8_t SATA_CONTR = 6;
  const uint8_t SERIAL_ATT_SCSI = 7;
  const uint8_t NVM_MEM_CONTR = 8;

  // Network controllers.
  const uint8_t ETHERNET_CONTR = 0;
  const uint8_t TOKEN_RING_CONTR = 1;
  const uint8_t FDDI_CONTR = 2;
  const uint8_t ATM_CONTR = 3;
  const uint8_t ISDN_CONTR = 4;
  const uint8_t WORLDFIP_CONTR = 5;
  const uint8_t PCIMG_214_CONTR = 6;
  const uint8_t INFINIBAND_NET_CONTR = 7;
  const uint8_t FABRIC_CONTR = 8;

  // Display controllers
  const uint8_t VGA_COMPAT_CONTR = 0; // Yes, I know there's an overlap with VGA_COMPAT_DEV higher up...
  const uint8_t XGA_CONTR = 1;
  const uint8_t THREE_D_CONTR = 2;

  // Multimedia controllers.
  const uint8_t MM_VIDEO_CONTR = 0;
  const uint8_t MM_AUDIO_CONTR = 1;
  const uint8_t MM_TELE_DEV = 2;
  const uint8_t MM_AUDIO_DEV = 3;

  // Memory controllers.
  const uint8_t RAM_CONTR = 0;
  const uint8_t FLASH_CONTR = 1;

  // Bridge devices;
  const uint8_t HOST_BRIDGE = 0;
  const uint8_t ISA_BRIDGE = 1;
  const uint8_t EISA_BRIDGE = 2;
  const uint8_t MCA_BRIDGE = 3;
  const uint8_t PCI_TO_PCI_BRIDGE = 4;
  const uint8_t PCMCIA_BRIDGE = 5;
  const uint8_t NUBUS_BRIDGE = 6;
  const uint8_t CARDBUS_BRIDGE = 7;
  const uint8_t RACEWAY_BRIDGE = 8;
  const uint8_t PCI_TO_PCI_ST = 9;
  const uint8_t INFINIBAND_TO_PCI_BRIDGE = 10;

  // Simple Communication controllers.
  const uint8_t SERIAL_CONTR = 0;
  const uint8_t PARALLEL_CONTR = 1;
  const uint8_t MULTIPORT_SERIAL_CONTR = 2;
  const uint8_t MODEM = 3;
  const uint8_t GPIB_CONTR = 4;
  const uint8_t SMART_CARD = 5;

  // Base system peripherals.
  const uint8_t PIC = 0;
  const uint8_t DMA_CONTR = 1;
  const uint8_t TIMER = 2;
  const uint8_t RTC_CONTR = 3;
  const uint8_t PCI_HOTPLUG_CONTR = 4;
  const uint8_t SD_HOST_CONTR = 5;
  const uint8_t IOMMU = 6;

  // Input device controllers.
  const uint8_t KEYBOARD_CONTR = 0;
  const uint8_t DIGITIZER_PEN = 1;
  const uint8_t MOUSE_CONTR = 2;
  const uint8_t SCANNER_CONTR = 3;
  const uint8_t GAMEPORT_CONTR = 4;

  // Docking stations
  const uint8_t DOCKING_STATION_GEN = 0;

  // Processors
  const uint8_t PROC_386 = 0;
  const uint8_t PROC_486 = 1;
  const uint8_t PROC_PENTIUM = 2;
  const uint8_t PROC_ALPHA = 0x10;
  const uint8_t PROC_POWER_PC = 0x20;
  const uint8_t PROC_MIPS = 0x30;
  const uint8_t COPROCESSOR = 0x40;

  // Serial Bus controllers
  const uint8_t FIREWIRE_CONTR = 0;
  const uint8_t ACCESS_BUS_CONTR = 1;
  const uint8_t SSA_CONTR = 2;
  const uint8_t USB_CONTR = 3;
  const uint8_t FIBRE_CHAN_CONTR = 4;
  const uint8_t SMBUS_CONTR = 5;
  const uint8_t INFINIBAND_BUS_CONTR = 6;
  const uint8_t IPMI_CONTR = 7;
  const uint8_t SERCOS_CONTR = 8;
  const uint8_t CANBUS_CONTR = 9;

  // Wireless controllers
  const uint8_t IRDA_CONTR = 0;
  const uint8_t CONSUMER_IR_CONTR = 1;
  const uint8_t RF_CONTR = 0x10;
  const uint8_t BLUETOOTH_CONTR = 0x11;
  const uint8_t BROADBAND_CONTR = 0x12;
  const uint8_t ETHERNET_CONTR_802_1A = 0x20;
  const uint8_t ETHERNET_CONTR_802_1B = 0x21;

  // 'Intelligent' controllers
  const uint8_t I20_CONTR = 0;

  // Satellite communication controllers
  const uint8_t SAT_TV_CONTR = 1;
  const uint8_t SAT_AUDIO_CONTR = 2;
  const uint8_t SAT_VOICE_CONTR = 3;
  const uint8_t SAT_DATA_CONTR = 4;

  // Encrption controllers
  const uint8_t NET_AND_COMP_ENCRYPT = 0;
  const uint8_t ENTERTAIN_ENCRYPT = 0x10;

  // Signal processing controllers
  const uint8_t DPIO_CONTR = 0;
  const uint8_t PERF_COUNTERS = 1;
  const uint8_t COMMS_SYNCHER = 0x10;
  const uint8_t SIGNAL_PROC_MGMT = 0x20;
};

namespace PCI_CAPABILITY_IDS
{
  const uint8_t PCI_POWER_MGMT = 1;
  const uint8_t AGP = 2;
  const uint8_t VITAL_PROD_DATA = 3;
  const uint8_t SLOT_IDENT = 4;
  const uint8_t MSI = 5;
  const uint8_t COMPACT_PCI_HOTSWAP = 6;
  const uint8_t PCI_X = 7;
  const uint8_t HYPERTRANSPORT = 8;
  const uint8_t VENDOR_SPECIFIC_CAP = 9;
  const uint8_t DEBUG_PORT = 10;
  const uint8_t COMPACT_PCI_CRC = 11;
  const uint8_t PCI_HOTPLUG = 12;
  const uint8_t PCI_BRIDGE_VENDOR_ID = 13;
  const uint8_t AGP_8X = 14;
  const uint8_t SECURE_DEVICE = 15;
  const uint8_t PCI_EXPRESS = 16;
  const uint8_t MSI_X = 17;
};