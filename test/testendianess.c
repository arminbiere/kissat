#include "test.h"

#include "../src/endianess.h"

#include <stdbool.h>

struct first
{
  bool bit:1;
  unsigned rest:31;
};

struct last
{
  unsigned rest:31;
  bool bit:1;
};

union type
{
  struct first first;
  struct last last;
  unsigned raw;
};

#define PRINT(EXPR) \
do { \
  const unsigned value = (unsigned)(EXPR); \
  printf ("%s == %08x\n", #EXPR, value); \
} while (0)

static void
test_endianess (void)
{
  assert (sizeof (struct first) == 4);
  assert (sizeof (struct last) == 4);
  assert (sizeof (union type) == 4);
  // *INDENT-OFF*
  PRINT (((union type) { .raw = 1u      }).raw);
  PRINT (((union type) { .raw = (1u<<31)}).raw);
  printf ("\n");
  PRINT (((union type) { .raw = 1u      }).first.bit);
  PRINT (((union type) { .raw = 1u      }).first.rest);
  PRINT (((union type) { .raw = (1u<<31)}).first.bit);
  PRINT (((union type) { .raw = (1u<<31)}).first.rest);
  printf ("\n");
  PRINT (((union type) { .raw = 1u      }).last.bit);
  PRINT (((union type) { .raw = 1u      }).last.rest);
  PRINT (((union type) { .raw = (1u<<31)}).last.bit);
  PRINT (((union type) { .raw = (1u<<31)}).last.rest);
  printf ("\n");
#ifdef KISSAT_IS_BIG_ENDIAN
  if (((union type) { .raw = 1u}).last.bit)
    printf ("big endian as expected\n");
  else if (((union type) { .raw = 1u}).first.bit)
    FATAL ("unexpected little endian");
#else
  if (((union type) { .raw = 1u}).first.bit)
    printf ("little endian as expected\n");
  else if (((union type) { .raw = 1u}).last.bit)
    FATAL ("unexpected big endian");
#endif
  else
    FATAL ("could not determine endianess");
  // *INDENT-ON*
}

void
tissat_schedule_endianess (void)
{
  SCHEDULE_FUNCTION (test_endianess);
}
