#ifndef __KLIB_PRINTF_TYPE_FN_H
#define __KLIB_PRINTF_TYPE_FN_H

int klib_snprintf(char *out_str, int max_out_len, const char *fmt, ...);
int klib_vsnprintf(char *out_str, int max_out_len, const char *fmt, va_list args);

#endif
