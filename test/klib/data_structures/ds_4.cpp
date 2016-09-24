#define ENABLE_TRACING

#include "klib/data_structures/string.h"
#include "klib/tracing/tracing.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <set>

using namespace std;

void data_structures_test_4()
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
}
