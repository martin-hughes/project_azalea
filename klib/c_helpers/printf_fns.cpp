// Provides the kernel's internal implementations of snprintf and vsnprintf.
// No direct printf type function is exposed - the kernel doesn't output anything directly.

#include "klib/klib.h"
#include "external/vsnprintf/vsnprintf.h"

int klib_snprintf(char *out_str, int max_out_len, const char *fmt, ...)
{
  KL_TRC_ENTRY;
  int retval;
  va_list args;

  va_start(args, *fmt);

  retval = klib_vsnprintf(out_str, max_out_len, fmt, args);

  va_end(args);

  KL_TRC_EXIT;

  return retval;
}

int klib_vsnprintf(char *out_str, int max_out_len, const char *fmt, va_list args)
{
  int retval;
  KL_TRC_ENTRY;

  retval = rpl_vsnprintf(out_str, max_out_len, fmt, args);

  KL_TRC_EXIT;
  return retval;
}
