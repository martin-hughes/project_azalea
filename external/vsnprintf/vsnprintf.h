#ifndef __EXT_VSNPRINTF_H
#define __EXT_VSNPRINTF_H

typedef unsigned long size_t;

#define HAVE_VSNPRINTF 0
#define HAVE_SNPRINTF 0
#define HAVE_VASPRINTF 0
#define HAVE_ASPRINTF 0
#define HAVE_STDARG_H 0
#define HAVE_STDDEF_H 0
#define HAVE_STDINT_H 0
#define HAVE_STDLIB_H 0
#define HAVE_INTTYPES_H 0
#define HAVE_LOCALE_H 0
#define HAVE_LOCALECONV 0
#define HAVE_LCONV_DECIMAL_POINT 0
#define HAVE_LCONV_THOUSANDS_SEP 0
#define HAVE_LONG_DOUBLE 0
#define HAVE_LONG_LONG_INT 0
#define HAVE_UNSIGNED_LONG_LONG_INT 0
#define HAVE_INTMAX_T 0
#define HAVE_UINTMAX_T 0
#define HAVE_UINTPTR_T 0
#define HAVE_PTRDIFF_T 0
#define HAVE_VA_COPY 0
#define HAVE___VA_COPY 0

const int INT_MAX = ~((int)0);

#define E2BIG 7

int rpl_vsnprintf(char *str, size_t size, const char *format, va_list args);


#endif
