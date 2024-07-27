#include "../src/keatures.h"

#include <stdbool.h>

#include "test.h"

struct head {
  bool bit : 1;
  unsigned rest : 31;
};

struct tail {
  unsigned rest : 31;
  bool bit : 1;
};

union type {
  struct head head;
  struct tail tail;
  unsigned raw;
};

#define PRINT(EXPR) \
  do { \
    const unsigned value = (unsigned) (EXPR); \
    printf ("%s == %08x\n", #EXPR, value); \
  } while (0)

static void test_endianness (void) {
  assert (sizeof (struct head) == 4);
  assert (sizeof (struct tail) == 4);
  assert (sizeof (union type) == 4);
  // clang-format off
  PRINT (((union type) { .raw = 1u      }).raw);
  PRINT (((union type) { .raw = (1u<<31)}).raw);
  printf ("\n");
  PRINT (((union type) { .raw = 1u      }).head.bit);
  PRINT (((union type) { .raw = 1u      }).head.rest);
  PRINT (((union type) { .raw = (1u<<31)}).head.bit);
  PRINT (((union type) { .raw = (1u<<31)}).head.rest);
  printf ("\n");
  PRINT (((union type) { .raw = 1u      }).tail.bit);
  PRINT (((union type) { .raw = 1u      }).tail.rest);
  PRINT (((union type) { .raw = (1u<<31)}).tail.bit);
  PRINT (((union type) { .raw = (1u<<31)}).tail.rest);
  printf ("\n");
#ifdef KISSAT_IS_BIG_ENDIAN
  if (((union type) { .raw = 1u}).tail.bit)
    printf ("big endian as expected\n");
  else if (((union type) { .raw = 1u}).head.bit)
    FATAL ("unexpected little endian");
#else
  if (((union type) { .raw = 1u}).head.bit)
    printf ("little endian as expected\n");
  else if (((union type) { .raw = 1u}).tail.bit)
    FATAL ("unexpected big endian");
#endif
  else
    FATAL ("could not determine endianness");
  // clang-format on
}

void tissat_schedule_endianness (void) {
  SCHEDULE_FUNCTION (test_endianness);
}
