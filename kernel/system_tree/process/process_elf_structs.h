#ifndef __ST_PROC_ELF_STRUCTS_H
#define __ST_PROC_ELF_STRUCTS_H

// Structures in this file are adapted from "ELF-64 Object File Format"

#pragma pack(push, 1)

const uint64_t ELF64_FILE_HDR_SIZE = 64;
const uint64_t ELF64_PROG_HDR_SIZE = 56;

typedef struct
{
  uint8_t ident[16];
  uint16_t type;
  uint16_t machine_type;
  uint32_t version;
  uint64_t entry_addr;
  uint64_t prog_hdrs_off;
  uint64_t sect_hdrs_off;
  uint32_t flags;
  uint16_t file_header_size;
  uint16_t prog_hdr_entry_size;
  uint16_t num_prog_hdrs;
  uint16_t sect_hdr_entry_size;
  uint16_t num_sect_hdrs;
  uint16_t sect_name_str_table_idx;
} elf64_file_header;

static_assert(sizeof(elf64_file_header) == ELF64_FILE_HDR_SIZE, "elf64_file_header size does not match");

typedef struct
{
  uint32_t type;
  uint32_t flags;
  uint64_t file_offset;
  uint64_t req_virt_addr;
  uint64_t req_phys_addr;
  uint64_t size_in_file;
  uint64_t size_in_mem;
  uint64_t req_alignment;
} elf64_program_header;

static_assert(sizeof(elf64_program_header) == ELF64_PROG_HDR_SIZE, "elf64_program_header size does not match");

#pragma pack(pop)

#endif
