#ifndef _KLIB_TRACING_H
#define _KLIB_TRACING_H

//#define ENABLE_TRACING

#define TRC_LVL_FATAL 100
#define TRC_LVL_ERROR 90
#define TRC_LVL_IMPORTANT 80
#define TRC_LVL_FLOW 60
#define TRC_LVL_EXTRA 10

#ifdef ENABLE_TRACING
#define KL_TRC_INIT_TRACING kl_tr_init_tracing
#define KL_TRC_TRACE(args) kl_trc_trace args

#define KL_TRC_ENTRY kl_trc_trace(TRC_LVL_FLOW, "ENTRY "); \
                     kl_trc_trace(TRC_LVL_FLOW, __FUNCTION__); \
                     kl_trc_trace(TRC_LVL_FLOW, " { \n")
#define KL_TRC_EXIT kl_trc_trace(TRC_LVL_FLOW, "EXIT "); \
                    kl_trc_trace(TRC_LVL_FLOW, __FUNCTION__); \
                    kl_trc_trace(TRC_LVL_FLOW, " } \n")

#define KL_TRC_DATA(name, val) kl_trc_trace(TRC_LVL_FLOW, (name)); \
                               kl_trc_trace(TRC_LVL_FLOW, ": "); \
                               kl_trc_trace(TRC_LVL_FLOW, (val)); \
                               kl_trc_trace(TRC_LVL_FLOW, "\n")

#else
#define KL_TRC_INIT_TRACING
#define KL_TRC_TRACE(args)
#define KL_TRC_DATA(name, val)

#define KL_TRC_ENTRY
#define KL_TRC_EXIT

#endif

// Function declarations. These should - largely - never be called directly, to
// allow for compile-time removal of tracing calls in the release build.
void kl_trc_init_tracing();
void kl_trc_trace(unsigned long level, const char *message);
void kl_trc_trace(unsigned long level, unsigned long value);

#endif
