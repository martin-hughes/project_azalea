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
	ASSERT(string_a == "Hello!");
  ASSERT(string_a != "Noooo!");

  kl_string string_b("Hello!");
  ASSERT(string_a == string_b);

  kl_string string_c("Byyyeee!");
  ASSERT(string_a != string_c);

  // Assignments
  string_a = "Go!";
  ASSERT(string_a == "Go!");
  ASSERT(string_b != string_a);

  string_a = string_c;
  ASSERT(string_a == "Byyyeee!");
  ASSERT(string_a == string_c);
  ASSERT(string_a != "Hello!");

  // Construction tests
  string_a = kl_string("Construct");
  ASSERT(string_a == "Construct");

  kl_string string_d;
  string_d = "Construct";
  ASSERT(string_a == string_d);

  kl_string string_e(string_d);
  ASSERT(string_a == string_e);

  // String additions
  kl_string string_f(string_a + string_b);
  ASSERT(string_f == "ConstructHello!");
  ASSERT(string_a == "Construct");
  ASSERT(string_b == "Hello!");

  string_f = string_a + "Bye!";
  ASSERT(string_f == "ConstructBye!");

  string_f = string_f + "Bye!";
  ASSERT(string_f == "ConstructBye!Bye!");

  // Subscripts
  ASSERT(string_f[2] == 'n');
  string_f[2] = 'o';
  ASSERT(string_f == "CoostructBye!Bye!");

  ASSERT(string_f < string_b);
  ASSERT(string_b > string_f);

  // Substrings
  string_a = "Hello cruel world";
  ASSERT(string_a.length() == 17);
  cout << "Position of 'cruel': " << string_a.find("cruel") << endl;
  ASSERT(string_a.find("cruel") == 6);
  ASSERT(string_a.find("Hello") == 0);
  ASSERT(string_a.find("NOOO") == kl_string::npos);
  ASSERT(string_a.find("world.") == kl_string::npos);
  ASSERT(string_a.find("Hello cruel world I hate everything") == kl_string::npos);

  ASSERT(string_a.substr(6, 5) == "cruel");
  ASSERT(string_a.substr(6, 0) == "");
  ASSERT(string_a.substr(6, kl_string::npos) == "cruel world");
  ASSERT(string_a.substr(6, 1000) == "cruel world");

  kl_string string_g;
  ASSERT(string_g == "");

  kl_string string_h("Hello!", 4);
  ASSERT_EQ(string_h, "Hell");
}
