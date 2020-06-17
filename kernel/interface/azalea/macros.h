/** @file
 *  @brief Compatibility macros
 */

#pragma once

/* Use these macros to make C and C++ compatible enums. We prefer enum class in C++ to prevent namespace pollution. */

/** @cond */
#ifdef __cplusplus

#define AZALEA_ENUM_CLASS class
#define AZALEA_RENAME_ENUM(X) typedef X ## _T X
#define INIT(x) { x }
#define OPT_STRUCT
#define ENUM_TAG(x, y) y

#else

#define AZALEA_ENUM_CLASS
#define AZALEA_RENAME_ENUM(X) typedef enum X ## _T X
#define INIT(x)
#define OPT_STRUCT struct
#define ENUM_TAG(x, y) x ## y

#endif

#define SC_MAX_WAIT (~0)

/** @endcond */
