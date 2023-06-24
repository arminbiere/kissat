#include "../src/utilities.h"

#include <inttypes.h>

#include "test.h"

#define TEST_LOG2_CEILING_OF_WORD(W, L) \
  do { \
    assert (kissat_log2_ceiling_of_word (W) == L); \
    printf ("kissat_log2_ceiling_of_word (%" PRIuPTR ") = %u\n", \
            (uintptr_t) (W), (unsigned) (L)); \
  } while (0)

static void test_log2_ceiling_of_word (void) {
  const unsigned bits = sizeof (word) * 8;
  const word bit = 1;
  TEST_LOG2_CEILING_OF_WORD (0, 0);
  TEST_LOG2_CEILING_OF_WORD (1, 0);
  TEST_LOG2_CEILING_OF_WORD (2, 1);
  for (unsigned i = 2; i < bits; i++)
    TEST_LOG2_CEILING_OF_WORD ((bit << i) - 1, i);
  for (unsigned i = 2; i < bits; i++)
    TEST_LOG2_CEILING_OF_WORD ((bit << i) + 0, i);
  for (unsigned i = 2; i < bits; i++)
    TEST_LOG2_CEILING_OF_WORD ((bit << i) + 1, i + 1);
  TEST_LOG2_CEILING_OF_WORD (~(word) 0, bits);
}

#define TEST_LEADING_ZEROES_OF_UNSIGNED(U, B) \
  do { \
    assert (kissat_leading_zeroes_of_unsigned (U) == B); \
    printf ("kissat_leading_zeroes_of_unsigned (%u) = %u\n", \
            (unsigned) (U), (unsigned) (B)); \
  } while (0)

static void test_leading_zeroes_of_unsigned (void) {
  TEST_LEADING_ZEROES_OF_UNSIGNED (0, 32);
  TEST_LEADING_ZEROES_OF_UNSIGNED (1, 31);
  TEST_LEADING_ZEROES_OF_UNSIGNED (2, 30);
  for (unsigned i = 2; i < 32; i++)
    TEST_LEADING_ZEROES_OF_UNSIGNED ((1u << i) - 1, 32 - i);
  for (unsigned i = 2; i < 32; i++)
    TEST_LEADING_ZEROES_OF_UNSIGNED ((1u << i) + 0, 31 - i);
  for (unsigned i = 2; i < 32; i++)
    TEST_LEADING_ZEROES_OF_UNSIGNED ((1u << i) + 1, 31 - i);
  TEST_LEADING_ZEROES_OF_UNSIGNED (~(unsigned) 0, 0);
}

#define TEST_LEADING_ZEROES_OF_WORD(W, B) \
  do { \
    assert (kissat_leading_zeroes_of_word (W) == B); \
    printf ("kissat_leading_zeroes_of_word (%" PRIuPTR ") = %u\n", \
            (uintptr_t) (W), (unsigned) (B)); \
  } while (0)

static void test_leading_zeroes_of_word (void) {
  const unsigned bits = sizeof (word) * 8;
  const word bit = 1;
  TEST_LEADING_ZEROES_OF_WORD (0, bits);
  TEST_LEADING_ZEROES_OF_WORD (1, bits - 1);
  TEST_LEADING_ZEROES_OF_WORD (2, bits - 2);
  for (unsigned i = 2; i < bits; i++)
    TEST_LEADING_ZEROES_OF_WORD ((bit << i) - 1, bits - i);
  for (unsigned i = 2; i < bits; i++)
    TEST_LEADING_ZEROES_OF_WORD ((bit << i) + 0, bits - i - 1);
  for (unsigned i = 2; i < bits; i++)
    TEST_LEADING_ZEROES_OF_WORD ((bit << i) + 1, bits - i - 1);
  TEST_LEADING_ZEROES_OF_WORD (~(word) 0, 0);
}

void tissat_schedule_ceil (void) {
  SCHEDULE_FUNCTION (test_leading_zeroes_of_unsigned);
  SCHEDULE_FUNCTION (test_leading_zeroes_of_word);
  SCHEDULE_FUNCTION (test_log2_ceiling_of_word);
}
