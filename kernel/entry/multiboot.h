/// @file
/// @brief Constants and structures to help with multiboot

#pragma once

const unsigned int MULTIBOOT_CONSTANT = 0x2BADB002; ///< Constant indicating a multiboot kernel.

/// @brief The multiboot header, as defined in the multiboot specification.
///
/// The x64 version of Azalea is compiled into a multiboot-compatible binary to be loaded by, e.g. GRUB.
///
/// Since the contents of this structure are defined in the multiboot spec, they are not documented again here.
struct multiboot_hdr
{
/// @cond
  unsigned int flags;

  unsigned int mem_lower;
  unsigned int mem_upper;

  unsigned int boot_device;

  unsigned int cmdline_ptr;

  // Modules table
  unsigned int mods_count;
  unsigned int mods_ptr;

  // Symbol table
  unsigned int tabsize;
  unsigned int strsize;
  unsigned int syms_addr;
  unsigned int reserved;

  // MMap table
  unsigned int mmap_length;
  unsigned int mmap_addr;

  // Drives table
  unsigned int drives_length;
  unsigned int drives_addr;

  unsigned int config_table_addr;

  unsigned int boot_loader_name_addr;

  unsigned int apm_table_addr;

  // VBE mode data
  unsigned int vbe_control_info_addr;
  unsigned int vbe_mode_info_addr;
  unsigned short vbe_mode;
  unsigned short vbe_interface_seg;
  unsigned short vbe_interface_off;
  unsigned short vbe_interface_len;

  // Graphics framebuffer info.
  unsigned long framebuffer_addr;
  unsigned int framebuffer_pitch;
  unsigned int framebuffer_width;
  unsigned int framebuffer_height;
  unsigned char framebuffer_bpp;
  unsigned char framebuffer_type;
  unsigned char color_info[6];
/// @endcond
} __attribute__((packed));

static_assert(sizeof(multiboot_hdr) == 116, "Multiboot header size wrong");
