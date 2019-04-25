/// @file
/// @brief ELF file loader.

#include <azalea/azalea.h>
#include <string.h>
#include <stdio.h>

//#define SC_DEBUG_MSG(string) \
//  syscall_debug_output((string), strlen((string)) )
#define SC_DEBUG_MSG(string)

/// @brief List of known ELF segment types.
///
namespace ELF_SEG
{
  const uint32_t NULL_SEG = 0; ///< NULL segment, can be skipped.
  const uint32_t LOAD = 1; ///< LOAD segment, loaded verbatim from disk.
  const uint32_t DYNAMIC = 2; ///< Not supported.
  const uint32_t INTERP = 3; ///< Not supported.
  const uint32_t NOTE = 4; ///< Not supported.
  const uint32_t SHLIB = 5; ///< Not supported.
  const uint32_t PHDR = 6; ///< Not supported.
  const uint32_t LO_PROC = 0x70000000; ///< Not supported.
  const uint32_t HI_PROC = 0x7fffffff; ///< Not supported.
  const uint32_t GNU_EH_FRAME = 0x6474E550;
  const uint32_t GNU_STACK = 0x6474E551;
  const uint32_t GNU_RELRO = 0x6474E552;
};

/// @brief Load the contents of an ELF file into a newly formed process.
///
/// This function will fail if proc_file is not a valid ELF file or process is not able to be written in to.
///
/// @param proc_file Handle for the file to read from.
///
/// @param process The process to load the file in to.
///
/// @return TBD.
ERR_CODE load_elf_file_in_process(GEN_HANDLE proc_file, GEN_HANDLE process)
{
  ERR_CODE result;
  elf64_file_header file_header;
  elf64_program_header prog_header;

  result = proc_read_elf_file_header(proc_file, &file_header);
  if (result != ERR_CODE::NO_ERROR)
  {
    return result;
  }
  for (uint32_t i = 0; i < file_header.num_prog_hdrs; i++)
  {
    SC_DEBUG_MSG("New section:\n");
    result = proc_read_elf_prog_header(proc_file, &file_header, &prog_header, i);
    SC_DEBUG_MSG(" - Header read\n");

    if (result != ERR_CODE::NO_ERROR)
    {
      break;
    }

    // At the moment, this is the only type that we'll load. Copy each section into memory.
    switch(prog_header.type)
    {
    case ELF_SEG::NULL_SEG:
      SC_DEBUG_MSG("NULL segment\n");
      break;

    case ELF_SEG::LOAD:
      SC_DEBUG_MSG("LOAD segment\n");
      result = proc_load_elf_load_segment(proc_file, process, prog_header);
      break;

    case ELF_SEG::DYNAMIC:
      SC_DEBUG_MSG("DYNAMIC segment\n");
      break;

    case ELF_SEG::INTERP:
      SC_DEBUG_MSG("INTERP segment\n");
      break;

    case ELF_SEG::NOTE:
      SC_DEBUG_MSG("NOTE segment\n");
      break;

    case ELF_SEG::SHLIB: // Always ignored apparently.
      SC_DEBUG_MSG("SHLIB segment - ignore\n");
      break;

    case ELF_SEG::PHDR:
      SC_DEBUG_MSG("PHDR segment\n");
      break;

    case ELF_SEG::LO_PROC:
      SC_DEBUG_MSG("LO_PROC segment\n");
      break;

    case ELF_SEG::HI_PROC:
      SC_DEBUG_MSG("HI_PROC segment\n");
      break;

    case ELF_SEG::GNU_EH_FRAME:
      SC_DEBUG_MSG("GNU EH Frame\n");
      break;

    case ELF_SEG::GNU_STACK:
      SC_DEBUG_MSG("GNU Stack info\n");
      break;

    case ELF_SEG::GNU_RELRO:
      SC_DEBUG_MSG("GNU Reload RO\n");
      break;

    default:
      SC_DEBUG_MSG("Unknown segment\n");
    }

    if (result != ERR_CODE::NO_ERROR)
    {
      break;
    }
  }

  return result;
}

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
    SC_DEBUG_MSG("Failed to read program header\n");
    return ERR_CODE::UNRECOGNISED;
  }

  result = syscall_get_handle_data_len(proc_file, &elf_file_size);
  if (result != ERR_CODE::NO_ERROR)
  {
    SC_DEBUG_MSG("Failed to get file size\n");
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
    SC_DEBUG_MSG("Not an ELF file\n");
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

/// @brief Load an ELF LOAD segment from disk into the specified process.
///
/// @param proc_file The file to load the segment from.
///
/// @param process The process to load the segment in to.
///
/// @param hdr The program header referencing the LOAD segment.
///
/// @return
ERR_CODE proc_load_elf_load_segment(GEN_HANDLE proc_file, GEN_HANDLE process, elf64_program_header &hdr)
{
  uint64_t end_addr;
  uint64_t copy_end_addr;
  uint64_t page_start_addr;
  uint64_t bytes_written = 0;
  uint64_t offset;
  uint32_t pages_reqd;
  void *page_ptr;
  char *write_ptr;
  ERR_CODE result = ERR_CODE::NO_ERROR;

  if(hdr.req_phys_addr >= 0x8000000000000000L)
  {
    result = ERR_CODE::UNRECOGNISED;
  }

  end_addr = hdr.req_virt_addr + hdr.size_in_mem;
  copy_end_addr = hdr.req_virt_addr + hdr.size_in_file;
  page_start_addr = hdr.req_virt_addr - (hdr.req_virt_addr % MEM_PAGE_SIZE);
  bytes_written = 0;
  pages_reqd = ((end_addr - page_start_addr) / MEM_PAGE_SIZE) + 1;
  offset = hdr.req_virt_addr % MEM_PAGE_SIZE;

  page_ptr = nullptr;
  result = syscall_allocate_backing_memory(pages_reqd, &page_ptr);
  if (result != ERR_CODE::NO_ERROR)
  {
    return result;
  }

  SC_DEBUG_MSG(" - Backing mem allocated\n");

  write_ptr = reinterpret_cast<char *>(page_ptr) + offset;

  memset(write_ptr, 0, hdr.size_in_mem);
  result = syscall_read_handle(proc_file,
                               hdr.file_offset,
                               hdr.size_in_file,
                               reinterpret_cast<unsigned char *>(write_ptr),
                               (pages_reqd * MEM_PAGE_SIZE) - offset,
                               &bytes_written);
  if (result != ERR_CODE::NO_ERROR)
  {
    return result;
  }

  if (bytes_written != hdr.size_in_file)
  {
    result = ERR_CODE::UNRECOGNISED;
  }

  SC_DEBUG_MSG(" - Section read\n");

  result = syscall_map_memory(process,
                              reinterpret_cast<void *>(page_start_addr),
                              pages_reqd * MEM_PAGE_SIZE,
                              0,
                              page_ptr);
  if (result != ERR_CODE::NO_ERROR)
  {
    return result;
  }

  result = syscall_release_backing_memory(page_ptr);
  if (result != ERR_CODE::NO_ERROR)
  {
    if (result == ERR_CODE::NOT_FOUND)
    {
      result = ERR_CODE::INVALID_OP;
    }
    return result;
  }

  SC_DEBUG_MSG(" - Memory finalised\n");

  return result;
}
