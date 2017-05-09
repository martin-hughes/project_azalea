#ifndef _KLIB_TRACING_H
#define _KLIB_TRACING_H

#include <type_traits>
#include "klib/data_structures/string.h"

//#define ENABLE_TRACING

enum class TRC_LVL
{
  EXTRA = 10,
  FLOW = 60,
  IMPORTANT = 80,
  ERROR = 90,
  FATAL = 100,
};

#ifdef ENABLE_TRACING
#define KL_TRC_INIT_TRACING kl_tr_init_tracing
#define KL_TRC_TRACE(...) kl_trc_trace(__VA_ARGS__)

#define KL_TRC_ENTRY kl_trc_trace(TRC_LVL::FLOW, "ENTRY ", __FUNCTION__, " { \n")
#define KL_TRC_EXIT kl_trc_trace(TRC_LVL::FLOW, "EXIT ", __FUNCTION__, " } \n")

#define KL_TRC_DATA(name, val) kl_trc_trace(TRC_LVL::FLOW, (name), ": ", (unsigned long)(val), "\n")

#else
#define KL_TRC_INIT_TRACING
#define KL_TRC_TRACE(...)
#define KL_TRC_DATA(name, val)

#define KL_TRC_ENTRY
#define KL_TRC_EXIT

#endif

// Function declarations. These should - largely - never be called directly, to
// allow for compile-time removal of tracing calls in the release build.
void kl_trc_init_tracing();

template<typename ... args_t> void kl_trc_trace(TRC_LVL lvl, args_t ... params);
template<typename p, typename ... args_t> void kl_trc_output_arguments(p param, args_t ... params);
template<typename p> void kl_trc_output_arguments(p param);
void kl_trc_output_arguments();

//template <typename p> void kl_trc_output_argument(p param);
void kl_trc_output_str_argument(char const *str);
void kl_trc_output_int_argument(unsigned long value);
void kl_trc_output_kl_string_argument(kl_string &str);

// Template to output integral types
template<typename T = unsigned long, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
T kl_trc_output_single_arg(T param)
{
  kl_trc_output_int_argument((unsigned long)param);
  return param;
}

// Templates to output string types
template<typename T = char const *, typename = typename std::enable_if<std::is_pointer<T>::value, T>::type,
    typename = typename std::enable_if<std::is_same<T, char const *>::value>::type, typename X = void>
T kl_trc_output_single_arg(T param)
{
  kl_trc_output_str_argument(param);
  return param;
}

template<typename T = kl_string, typename = typename std::enable_if<std::is_same<T, kl_string>::value>::type, typename B = void, typename C = void, typename D = void, typename E = void>
T kl_trc_output_single_arg(T& param)
{
  kl_trc_output_kl_string_argument(param);
  return param;
}

// Template to output all other pointers
template<typename T, typename = typename std::enable_if<std::is_pointer<T>::value, T>::type,
    typename = typename std::enable_if<!std::is_same<T, char const *>::value>::type, typename X = void,
    typename Y = void>
T kl_trc_output_single_arg(T param)
{
  kl_trc_output_int_argument((unsigned long)param);
  return param;
}

// The actual tracing function. Notice that lvl is ignored for the time being!
template<typename ... args_t> void kl_trc_trace(TRC_LVL lvl, args_t ... params)
{
  kl_trc_output_arguments(params...);
}

template<typename p, typename ... args_t> void kl_trc_output_arguments(p param, args_t ... params)
{
  kl_trc_output_single_arg(param);
  kl_trc_output_arguments(params...);
}

template<typename p> void kl_trc_output_arguments(p param)
{
  kl_trc_output_single_arg(param);
}

#endif
