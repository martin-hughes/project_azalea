// Configure the x64 PAT register, and encode / decode between it and the page table entries.
// Nothing special occurs here, all the values are fixed, so just use simple lookups.

#include "mem/mem.h"
#include "klib/klib.h"
#include "mem_internal_x64.h"
#include "processor/x64/processor-x64-int.h"

const unsigned long PAT_REGISTER_VAL = 0x0005040600010406;

// Configure that PAT as follows:
// PAT 0: Write back (default)
// PAT 1: Write through (default)
// PAT 2: Write combining (not the default of UC-)
// PAT 3: Uncacheable (default)
// PAT 4: Write back (default)
// PAT 5: Write through (default)
// PAT 6: Write protected (not the dault of UC-)
// PAT 7: Uncacheable (default)
//
// This table is encoded in PAT_REGISTER_VAL, above.
void mem_x64_pat_init()
{
  KL_TRC_ENTRY;
  asm_proc_write_msr(PROC_X64_MSRS::IA32_PAT, PAT_REGISTER_VAL);
  KL_TRC_EXIT;
}

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
