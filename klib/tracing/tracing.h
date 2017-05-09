#ifndef _KLIB_TRACING_H
#define _KLIB_TRACING_H

#include <type_traits>

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

template <typename... args_t> void kl_trc_trace(TRC_LVL lvl, args_t... params);
template <typename p, typename... args_t> void kl_trc_output_argument(p param, args_t... params);
//template <typename p> void kl_trc_output_argument(p param);
void kl_trc_output_argument(char const *str);
void kl_trc_output_argument(unsigned long value);
void kl_trc_output_argument();

template <typename... args_t> void kl_trc_trace(TRC_LVL lvl, args_t... params)
{
  kl_trc_output_argument(params...);
}

template <typename p, typename... args_t> void kl_trc_output_argument(p param, args_t... params)
{
  if (std::is_integral<p>::value)
  {
    kl_trc_output_argument((unsigned long)param);
  }
  else
  {
    kl_trc_output_argument((char const *)param);
  }
  kl_trc_output_argument(params...);
}

#endif
