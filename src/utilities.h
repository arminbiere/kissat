#ifndef _utilities_h_INCLUDED
#define _utilities_h_INCLUDED

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uintptr_t word;

#define ASSUMED_LD_CACHE_LINE_SIZE 7

static inline word
kissat_cache_lines (word n, size_t size)
{
  (void) size;
  assert (size == 4);
  assert (ASSUMED_LD_CACHE_LINE_SIZE > 2);
  return n >> (ASSUMED_LD_CACHE_LINE_SIZE - 2);
}

#define WORD_ALIGNMENT_MASK (sizeof (word)-1)
#define WORD_FORMAT PRIuPTR

#define MAX_SIZE_T (~ (size_t) 0)

static inline double
kissat_average (double a, double b)
{
  return b ? a / b : 0.0;
}

static inline double
kissat_percent (double a, double b)
{
  return kissat_average (100.0 * a, b);
}

static inline bool
kissat_aligned_word (word word)
{
  return !(word & WORD_ALIGNMENT_MASK);
}

static inline bool
kissat_aligned_pointer (const void *p)
{
  return kissat_aligned_word ((word) p);
}

static inline word
kissat_align_word (word w)
{
  word res = w;
  if (res & WORD_ALIGNMENT_MASK)
    res = 1 + (res | WORD_ALIGNMENT_MASK);
  return res;
}

bool kissat_has_suffix (const char *str, const char *suffix);

static inline bool
kissat_is_power_of_two (uint64_t w)
{
  return w && !(w & (w - 1));
}

static inline bool
kissat_is_zero_or_power_of_two (word w)
{
  return !(w & (w - 1));
}

unsigned kissat_ldceil (word);

#define SWAP(TYPE,A,B) \
do { \
  TYPE TMP_SWAP = (A); \
  (A) = (B); \
  (B) = (TMP_SWAP); \
} while (0)

#define MIN(A,B) \
  ((A) > (B)  ? (B) : (A))

#define MAX(A,B) \
  ((A) < (B)  ? (B) : (A))

#define ABS(A) \
  (assert ((int)(A) != INT_MIN), (A) < 0 ? -(A) : (A))

#endif
