#ifndef __KLIB_VARGS_H
#define __KLIB_VARGS_H

// KLIB Variable Arguments List support. Relies heavily on the GCC builtins!

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
