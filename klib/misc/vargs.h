#ifndef __KLIB_VARGS_H
#define __KLIB_VARGS_H

// KLIB Variable Arguments List support. Relies heavily on the GCC builtins!

// Ordinarily we'd not check against WIN32, but putting this here allows the tests code to be built in Visual Studio,
// and I find debugging in Visual Studio much easier than Eclipse!
#ifndef WIN32

typedef __builtin_va_list       va_list;

#define va_start(ap, last) \
          __builtin_va_start((ap), (last))

#define va_arg(ap, type) \
         __builtin_va_arg((ap), type)

#define __va_copy(dest, src) \
        __builtin_va_copy((dest), (src))

#define va_end(ap) \
        __builtin_va_end(ap)

#endif

#endif
