#ifndef __KLIB_VARGS_H
#define __KLIB_VARGS_H

// KLIB Variable Arguments List support. Relies heavily on the GCC builtins!

// If we allow the vargs macros to be defined other than just for the main Azalea kernel then any accidental inclusion
// of klib in the test code causes duplicate definition errors.
#ifndef WIN32
#ifndef _WIN64
#ifndef _LINUX
#ifndef AZALEA_TEST_CODE

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
#endif
#endif

#endif
