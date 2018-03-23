/// @file
/// @brief Configure the x64 PAT register, and encode / decode between it and the page table entries.
///
/// Nothing fancy occurs in this file, all the values are fixed, so just use simple lookups.

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/x64/mem-x64.h"
#include "mem/x64/mem-x64-int.h"
#include "processor/x64/processor-x64.h"
#include "processor/x64/processor-x64-int.h"

const unsigned long PAT_REGISTER_VAL = 0x0005040600010406;

/// @brief Initialise the PAT
///
/// Configure that PAT entries as follows:
/// 0. Write back (default)
/// 1. Write through (default)
/// 2. Write combining (not the default of UC-)
/// 3. Uncacheable (default)
/// 4. Write back (default)
/// 5. Write through (default)
/// 6. Write protected (not the default of UC-)
/// 7. Uncacheable (default)
///
/// This entire table is encoded in PAT_REGISTER_VAL.
void mem_x64_pat_init()
{
  KL_TRC_ENTRY;
  proc_write_msr(PROC_X64_MSRS::IA32_PAT, PAT_REGISTER_VAL);
  KL_TRC_EXIT;
}

/// @brief For a given required caching type, return the index in the PAT that would fulfil it.
///
/// If, for any reason, the request cannot be fulfilled, the system will panic.
///
/// @param cache_type The required caching mode. Must be one of MEM_X64_CACHE_TYPES.
///
/// @param first_half Whether or not the returned value must be three or less, since some parts of the page tables only
///                   support small values.
///
/// @return The index into the PAT that represents the requested caching mode.
unsigned char mem_x64_pat_get_val(const unsigned char cache_type, bool first_half)
{
  KL_TRC_ENTRY;

  unsigned char result;

  KL_TRC_DATA("Requested cache type", cache_type);
  KL_TRC_DATA("Must be first half?", first_half);

  switch(cache_type)
  {
    case MEM_X64_CACHE_TYPES::UNCACHEABLE:
      result = 3;
      break;

    case MEM_X64_CACHE_TYPES::WRITE_COMBINING:
      result = 2;
      break;

    case MEM_X64_CACHE_TYPES::WRITE_THROUGH:
      result = 1;
      break;

    case MEM_X64_CACHE_TYPES::WRITE_PROTECTED:
      ASSERT(!first_half);
      result = 6;
      break;

    case MEM_X64_CACHE_TYPES::WRITE_BACK:
      result = 0;
      break;

    default:
      panic("Invalid cache request");
  }

  KL_TRC_DATA("Result", result);
  return result;
}

/// @brief Convert and index into the PAT back into a cache type
///
/// @param pat_idx The index in the PAT to decode. Must be in the range 0-7, inclusive, or the system will panic.
///
/// @return The caching mode that PAT entry represents.
unsigned char mem_x64_pat_decode(const unsigned char pat_idx)
{
  KL_TRC_ENTRY;

  unsigned char result;

  ASSERT(pat_idx < 7);

  switch (pat_idx)
  {
    case 0:
      result = MEM_X64_CACHE_TYPES::WRITE_BACK;
      break;

    case 1:
      result = MEM_X64_CACHE_TYPES::WRITE_THROUGH;
      break;

    case 2:
      result = MEM_X64_CACHE_TYPES::WRITE_COMBINING;
      break;

    case 3:
      result = MEM_X64_CACHE_TYPES::UNCACHEABLE;
      break;

    case 4:
      result = MEM_X64_CACHE_TYPES::WRITE_BACK;
      break;

    case 5:
      result = MEM_X64_CACHE_TYPES::WRITE_THROUGH;
      break;

    case 6:
      result = MEM_X64_CACHE_TYPES::WRITE_PROTECTED;
      break;

    case 7:
      result = MEM_X64_CACHE_TYPES::UNCACHEABLE;
      break;
  }

  KL_TRC_EXIT;
  return result;
}
