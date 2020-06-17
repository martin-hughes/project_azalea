/// @file
/// @brief Kernel tracing functions

#pragma once

#include <string>

#include <stdint.h>
#include <type_traits>
#include "azalea/error_codes.h"

enum class OPER_STATUS_T;

//#define ENABLE_TRACING

// None of these functions are documented because they're reasonable self-explanatory.
/// @cond

#ifdef KL_TRACE_INCLUDE_TID
#include "pthread.h"
#endif

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
#define KL_TRC_ENABLE_OUTPUT kl_trc_enable_output
#define KL_TRC_DISABLE_OUTPUT kl_trc_disable_output

#define KL_TRC_TRACE(...) kl_trc_trace(__VA_ARGS__)

#define KL_TRC_ENTRY kl_trc_trace(TRC_LVL::FLOW, "ENTRY ", __FUNCTION__, " { \n")
#define KL_TRC_EXIT kl_trc_trace(TRC_LVL::FLOW, "EXIT ", __FUNCTION__, " } \n")

#else
#define KL_TRC_INIT_TRACING()
#define KL_TRC_ENABLE_OUTPUT()
#define KL_TRC_DISABLE_OUTPUT()

#define KL_TRC_TRACE(...)

#define KL_TRC_ENTRY
#define KL_TRC_EXIT

#endif

// Function declarations. These should - largely - never be called directly, to
// allow for compile-time removal of tracing calls in the release build.
void kl_trc_init_tracing();

void kl_trc_enable_output();
void kl_trc_disable_output();

template<typename ... args_t> void kl_trc_trace(TRC_LVL lvl, args_t ... params);
template<typename p, typename ... args_t> void kl_trc_output_arguments(p param, args_t ... params);
template<typename p> void kl_trc_output_arguments(p param);
void kl_trc_output_arguments();
void kl_trc_char(unsigned char c);

//template <typename p> void kl_trc_output_argument(p param);
void kl_trc_output_str_argument(char const *str);
void kl_trc_output_int_argument(uint64_t value);
void kl_trc_output_std_string_argument(std::string &str);
void kl_trc_output_err_code_argument(ERR_CODE ec);
void kl_trc_output_dev_status_argument(OPER_STATUS_T ds);

// Template to output integral types
template<typename T = uint64_t, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
T kl_trc_output_single_arg(T param)
{
  kl_trc_output_int_argument((uint64_t)param);
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

template<typename T = std::string, typename = typename std::enable_if<std::is_same<T, std::string>::value>::type,
    typename B = void, typename C = void, typename D = void, typename E = void>
T kl_trc_output_single_arg(T& param)
{
  kl_trc_output_std_string_argument(param);
  return param;
}

// Template to output all other pointers
template<typename T, typename = typename std::enable_if<std::is_pointer<T>::value, T>::type,
    typename = typename std::enable_if<!std::is_same<T, char const *>::value>::type, typename X = void,
    typename Y = void>
T kl_trc_output_single_arg(T param)
{
  kl_trc_output_int_argument((uint64_t)param);
  return param;
}

// Template to output ERR_CODE results
template<typename T, typename = typename std::enable_if<std::is_same<T, ERR_CODE>::value>::type,
    typename B = void, typename C = void, typename D = void, typename E = void, typename F = void>
T kl_trc_output_single_arg(T param)
{
  kl_trc_output_err_code_argument(param);
  return param;
}

// Template to output OPER_STATUS_T results
template<typename T, typename = typename std::enable_if<std::is_same<T, OPER_STATUS_T>::value>::type,
    typename B = void, typename C = void, typename D = void, typename E = void, typename F = void, typename G = void>
T kl_trc_output_single_arg(T param)
{
  kl_trc_output_dev_status_argument(param);
  return param;
}

// The actual tracing function. Notice that lvl is ignored for the time being!
template<typename ... args_t> void kl_trc_trace(TRC_LVL lvl, args_t ... params)
{
#ifdef KL_TRACE_INCLUDE_TID
  kl_trc_output_single_arg (pthread_self());
  kl_trc_output_single_arg (": ");
#endif
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

/// @endcond
