/** @file
 *  @brief Azalea's process management functionality (user mode part)
 */

#pragma once

#include <azalea/error_codes.h>
#include <azalea/kernel_types.h>
#include <azalea/macros.h>

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push, 1)

static const uint64_t ELF64_FILE_HDR_SIZE = 64; /**< ELF64 file header size.*/
static const uint64_t ELF64_PROG_HDR_SIZE = 56; /**< ELF64 program header size.*/

/** @brief an ELF64 file header.
 *
 * Details are contained in the ELF spec, so are not repeated.
 */
struct elf64_file_header
{
/** @cond */
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
/** @endcond */
};

/* Doing this kind of static assert in C doesn't seem to work very well, and it's not a terribly useful check most of
 * the time */
#ifdef __cplusplus
static_assert(sizeof(elf64_file_header) == ELF64_FILE_HDR_SIZE, "elf64_file_header size does not match");
#endif

/** An ELF64 program header
 *
 * Details are contained in the ELF spec, so are not repeated.
 */
struct elf64_program_header
{
/** @cond */
  uint32_t type;
  uint32_t flags;
  uint64_t file_offset;
  uint64_t req_virt_addr;
  uint64_t req_phys_addr;
  uint64_t size_in_file;
  uint64_t size_in_mem;
  uint64_t req_alignment;
/** @endcond */
};

#ifdef __cplusplus
static_assert( sizeof(elf64_program_header) == ELF64_PROG_HDR_SIZE , "elf64_program_header size does not match");
#endif

#pragma pack(pop)

ERR_CODE exec_file(const char *filename,
                   uint16_t name_length,
                   GEN_HANDLE *proc_handle,
                   char * const argv[],
                   char * const envp[]);
ERR_CODE load_elf_file_in_process(GEN_HANDLE proc_file, GEN_HANDLE process);
ERR_CODE proc_read_elf_file_header(GEN_HANDLE proc_file, OPT_STRUCT elf64_file_header *header);
ERR_CODE proc_read_elf_prog_header(GEN_HANDLE proc_file,
                                   OPT_STRUCT elf64_file_header *file_header,
                                   OPT_STRUCT elf64_program_header *prog_header,
                                   uint32_t index);

#ifdef __cplusplus
ERR_CODE proc_load_elf_load_segment(GEN_HANDLE proc_file, GEN_HANDLE process, OPT_STRUCT elf64_program_header &hdr);
#endif

#ifdef __cplusplus
};
#endif