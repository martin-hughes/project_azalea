#ifndef __ST_PROC_ELF_STRUCTS_H
#define __ST_PROC_ELF_STRUCTS_H

// Structures in this file are adapted from "ELF-64 Object File Format"

#pragma pack(push, 1)

const unsigned long ELF64_FILE_HDR_SIZE = 64;
const unsigned long ELF64_PROG_HDR_SIZE = 56;

typedef struct
{
  unsigned char ident[16];
  unsigned short type;
  unsigned short machine_type;
  unsigned int version;
  unsigned long entry_addr;
  unsigned long prog_hdrs_off;
  unsigned long sect_hdrs_off;
  unsigned int flags;
  unsigned short file_header_size;
  unsigned short prog_hdr_entry_size;
  unsigned short num_prog_hdrs;
  unsigned short sect_hdr_entry_size;
  unsigned short num_sect_hdrs;
  unsigned short sect_name_str_table_idx;
} elf64_file_header;

static_assert(sizeof(elf64_file_header) == ELF64_FILE_HDR_SIZE, "elf64_file_header size does not match");

typedef struct
{
  unsigned int type;
  unsigned int flags;
  unsigned long file_offset;
  unsigned long req_virt_addr;
  unsigned long req_phys_addr;
  unsigned long size_in_file;
  unsigned long size_in_mem;
  unsigned long req_alignment;
} elf64_program_header;

static_assert(sizeof(elf64_program_header) == ELF64_PROG_HDR_SIZE, "elf64_program_header size does not match");

#pragma pack(pop)

#endif
