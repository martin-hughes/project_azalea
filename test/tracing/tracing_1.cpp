#define ENABLE_TRACING

#include "test/test_core/test.h"
#include <iostream>
#include "gtest/gtest.h"


#include "klib/tracing/tracing.h"
#include "klib/data_structures/string.h"

using namespace std;

// Some simple tests to ensure that all types are traced in a reasonable way.
TEST(TracingTest, SimpleTracing)
{
  char a = 1;
  short b = 2;
  int c = 3;
  long d = 4;
  unsigned char e = 5;
  unsigned short f = 6;
  unsigned int g = 7;
  unsigned long h = 8;
  char const * i = "A string!!";
  kl_string j("Another string");
  void *k = reinterpret_cast<void *>(9);

  cout << "Values should appear below:" << endl;
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, a, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, b, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, c, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, d, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, e, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, f, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, g, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, h, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, i, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, j, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, k, "\n");
}
