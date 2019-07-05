#define ENABLE_TRACING

#include "test/test_core/test.h"
#include "klib/data_structures/string.h"
#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <set>
#include "gtest/gtest.h"

using namespace std;

// Tests of kl_string
TEST(DataStructuresTest, Strings1)
{
	kl_string string_a("Hello!");

  // Comparisons against kl_string and char *
	ASSERT_EQ(string_a, "Hello!");
  ASSERT_NE(string_a, "Noooo!");

  kl_string string_b("Hello!");
  ASSERT_EQ(string_a, string_b);

  kl_string string_c("Byyyeee!");
  ASSERT_NE(string_a, string_c);

  // Assignments
  string_a = "Go!";
  ASSERT_EQ(string_a, "Go!");
  ASSERT_NE(string_b, string_a);

  string_a = string_c;
  ASSERT_EQ(string_a, "Byyyeee!");
  ASSERT_EQ(string_a, string_c);
  ASSERT_NE(string_a, "Hello!");

  // Construction tests
  string_a = kl_string("Construct");
  ASSERT_EQ(string_a, "Construct");

  kl_string string_d;
  string_d = "Construct";
  ASSERT_EQ(string_a, string_d);

  kl_string string_e(string_d);
  ASSERT_EQ(string_a, string_e);

  // String additions
  kl_string string_f(string_a + string_b);
  ASSERT_EQ(string_f, "ConstructHello!");
  ASSERT_EQ(string_a, "Construct");
  ASSERT_EQ(string_b, "Hello!");

  string_f = string_a + "Bye!";
  ASSERT_EQ(string_f, "ConstructBye!");

  string_f = string_f + "Bye!";
  ASSERT_EQ(string_f, "ConstructBye!Bye!");

  // Subscripts
  ASSERT_EQ(string_f[2], 'n');
  string_f[2] = 'o';
  ASSERT_EQ(string_f, "CoostructBye!Bye!");

  ASSERT_LT(string_f, string_b);
  ASSERT_GT(string_b, string_f);

  // Substrings
  string_a = "Hello cruel world";
  ASSERT_EQ(string_a.length(), 17);
  cout << "Position of 'cruel': " << string_a.find("cruel") << endl;
  ASSERT_EQ(string_a.find("cruel"), 6);
  ASSERT_EQ(string_a.find("Hello"), 0);
  ASSERT_EQ(string_a.find("NOOO"), (uint64_t)kl_string::npos);
  ASSERT_EQ(string_a.find("world."), (uint64_t)kl_string::npos);
  ASSERT_EQ(string_a.find("Hello cruel world I hate everything"), (uint64_t)kl_string::npos);

  ASSERT_EQ(string_a.find_last("el"), 9);
  ASSERT_EQ(string_a.find_last("NOOO"), (uint64_t)kl_string::npos);
  ASSERT_EQ(string_a.find_last("Hello cruel world I hate everything"), (uint64_t)kl_string::npos);

  ASSERT_EQ(string_a.substr(6, 5), "cruel");
  ASSERT_EQ(string_a.substr(6, 0), "");
  ASSERT_EQ(string_a.substr(6, kl_string::npos), "cruel world");
  ASSERT_EQ(string_a.substr(6, 1000), "cruel world");

  kl_string string_g;
  ASSERT_EQ(string_g, "");

  kl_string string_h("Hello!", 4);
  ASSERT_EQ(string_h, "Hell");
}
