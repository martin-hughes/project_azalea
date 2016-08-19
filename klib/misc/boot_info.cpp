#include "boot_info.h"

const void **boot_info_acpi_tables = (const void **)0xFFFFFFFF00005000;
const unsigned int *boot_info_bsp_id = (unsigned int *)0xFFFFFFFF00005008;
const unsigned short *boot_info_cpu_speed =(unsigned short *)0xFFFFFFFF00005010;
const unsigned short *boot_info_cpu_cores_active =
    (unsigned short *)0xFFFFFFFF00005012;
const unsigned short *boot_info_cpu_cores_detd =
    (unsigned short *)0xFFFFFFFF00005014;
const unsigned short *boot_info_ram_megs= (unsigned short *)0xFFFFFFFF00005020;
const unsigned char *boot_info_mbr_flag = (unsigned char *)0xFFFFFFFF00005030;
const unsigned char *boot_info_ioapic_count =
    (unsigned char *)0xFFFFFFFF00005031;
const unsigned int *boot_info_video_base_addr =
    (unsigned int*)0xFFFFFFFF00005040;
const unsigned short *boot_info_video_x_pix =
    (unsigned short *)0xFFFFFFFF00005044;
const unsigned short *boot_info_video_y_pix =
    (unsigned short *)0xFFFFFFFF00005020;
const unsigned long *boot_info_ioapic_addrs =
    (unsigned long *)0xFFFFFFFF00005068;
const unsigned char *boot_info_lapic_ids = (unsigned char *)0xFFFFFFFF00005100;

const e820_record *e820_memory_map = (e820_record *)0xFFFFFFFF00004000;
