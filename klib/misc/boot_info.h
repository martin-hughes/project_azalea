// Simple header to provide access to the toot information provided by the
// Pure64 bootloader.

#ifndef _BOOT_INFO_HDR
#define _BOOT_INFO_HDR

#pragma pack(push,1)
struct e820_record
{
  unsigned long start_addr;
  unsigned long length;
  unsigned int memory_type;
  unsigned int __spare;
};
#pragma pack(pop)

extern const void **boot_info_acpi_tables;
extern const unsigned int *boot_info_bsp_id;
extern const unsigned short *boot_info_cpu_speed;
extern const unsigned short *boot_info_cpu_cores_active;
extern const unsigned short *boot_info_cpu_cores_detd;
extern const unsigned short *boot_info_ram_megs;
extern const unsigned char *boot_info_mbr_flag;
extern const unsigned char *boot_info_ioapic_count;
extern const unsigned int *boot_info_video_base_addr;
extern const unsigned short *boot_info_video_x_pix;
extern const unsigned short *boot_info_video_y_pix;
extern const unsigned long *boot_info_ioapic_addrs;
extern const unsigned char *boot_info_lapic_ids;

extern const e820_record *e820_memory_map;

#endif
