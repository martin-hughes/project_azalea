#ifndef KL_STRING_FNS_H
#define KL_STRING_FNS_H

#include <stdint.h>

uint64_t kl_strlen(const char *str, uint64_t max_len);
int32_t kl_strcmp(const char *str_a, const uint64_t max_len_a, const char *str_b, const uint64_t max_len_b);

#endif
