#ifndef __KLIB_VARGS_H
#define __KLIB_VARGS_H

// KLIB Variable Arguments List support.
// This code comes verbatim from the ACPICA sources, under a BSD licence, and is then modified for clarity.

#ifndef va_arg

#ifndef _VALIST
#define _VALIST
typedef char *va_list;
#endif /* _VALIST */

/* Storage alignment properties */

#define  _AUPBND                (sizeof (ACPI_NATIVE_INT) - 1)
#define  _ADNBND                (sizeof (ACPI_NATIVE_INT) - 1)

/* Variable argument list macro definitions */

#define _Bnd(X, bnd)            (((sizeof (X)) + (bnd)) & (~(bnd)))
#define va_arg(ap, T)           (*(T *)(((ap) += (_Bnd (T, _AUPBND))) - (_Bnd (T,_ADNBND))))
#define va_end(ap)              (ap = (va_list) NULL)
#define va_start(ap, A)         (void) ((ap) = (((char *) &(A)) + (_Bnd (A,_AUPBND))))

#endif /* va_arg */

#endif
