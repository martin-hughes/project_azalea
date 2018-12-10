/// @file
/// @brief ELF file loading helper functions

#include <azalea/azalea.h>

/// @brief Read the file header of an ELF executable file.
///
/// @param proc_file Handle of the ELF file to read.
///
/// @param header[out] Pointer to the header.
///
/// @return ERR_CODE::UNRECOGNISED if the file isn't an ELF file we understand. ERR_CODE::INVALID_PARAM if header ==
///         nullptr. ERR_CODE::NO_ERROR if the header was read successfully.
ERR_CODE proc_read_elf_file_header(GEN_HANDLE proc_file, elf64_file_header *header)
{
  ERR_CODE result;
  uint64_t bytes_read;
  uint64_t elf_file_size;

  if (header == nullptr)
  {
    return ERR_CODE::INVALID_PARAM;
  }

  result = syscall_read_handle(proc_file,
                               0,
                               sizeof(elf64_file_header),
                               reinterpret_cast<unsigned char *>(header),
                               sizeof(elf64_file_header),
                               &bytes_read);

  if ((result != ERR_CODE::NO_ERROR) || (bytes_read != sizeof(elf64_file_header)))
  {
    return ERR_CODE::UNRECOGNISED;
  }

  result = syscall_get_handle_data_len(proc_file, &elf_file_size);
  if (result != ERR_CODE::NO_ERROR)
  {
    return result;
  }

  if (!((header->ident[0] == 0x7f) &&
        (header->ident[1] == 'E') &&
        (header->ident[2] == 'L') &&
        (header->ident[3] == 'F')) ||
      (header->ident[4] != 2) || // 64-bit ELF.
      (header->ident[5] != 1) || // Little-endian.
      (header->ident[6] != 1) || // ELF version 1.
      (header->type != 2) || // Executable.
      (header->version != 1) || // ELF version 1 (again!)
      !((header->prog_hdrs_off > 0) && (header->prog_hdrs_off < (elf_file_size - ELF64_PROG_HDR_SIZE))) ||
      (header->num_prog_hdrs == 0) ||
      (header->entry_addr >= 0x8000000000000000LL) ||
      (header->file_header_size < ELF64_FILE_HDR_SIZE) ||
      (header->prog_hdr_entry_size < ELF64_PROG_HDR_SIZE))
  {
    return ERR_CODE::UNRECOGNISED;
  }

  return ERR_CODE::NO_ERROR;
}

/// @brief Read a program header from an ELF file.
///
/// @param proc_file Handle to the ELF file to interrogate.
///
/// @param file_header[in] Pointer to the file header of the ELF file.
///
/// @param prog_header[out] Pointer to storage for the program header to read.
///
/// @param index Which program header to read.
///
/// @return ERR_CODE::UNRECOGNISED if the file isn't an ELF format file. ERR_CODE::NO_ERROR if the program header was
///         read successfully. Or any other suitable error code.
ERR_CODE proc_read_elf_prog_header(GEN_HANDLE proc_file,
                                   elf64_file_header *file_header,
                                   elf64_program_header *prog_header,
                                   uint32_t index)
{
  ERR_CODE result;
  uint64_t bytes_read;

  if ((file_header == nullptr) || (prog_header == nullptr))
  {
    return ERR_CODE::INVALID_PARAM;
  }

  uint64_t prog_header_offset = (file_header->prog_hdrs_off + (index * (file_header->prog_hdr_entry_size)));

  result = syscall_read_handle(proc_file,
                               prog_header_offset,
                               sizeof(elf64_program_header),
                               reinterpret_cast<unsigned char *>(prog_header),
                               sizeof(elf64_program_header),
                               &bytes_read);

  if (result != ERR_CODE::NO_ERROR)
  {
    return result;
  }

  if (bytes_read != sizeof(elf64_program_header))
  {
    return ERR_CODE::UNRECOGNISED;
  }

  return ERR_CODE::NO_ERROR;
}
