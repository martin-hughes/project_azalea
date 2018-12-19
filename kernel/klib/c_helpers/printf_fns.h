#ifndef __KLIB_PRINTF_TYPE_FN_H
#define __KLIB_PRINTF_TYPE_FN_H

#ifdef AZALEA_TEST_CODE
#include <cstdarg>
#endif

#include <stdint.h>

uint32_t klib_snprintf(char *out_str, uint64_t max_out_len, const char *fmt, ...);
uint32_t klib_vsnprintf(char *out_str, uint64_t max_out_len, const char *fmt, va_list args);

#endif
