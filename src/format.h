#ifndef _format_h_INCLUDED
#define _format_h_INCLUDED

#include "utilities.h"

#include <stdbool.h>
#include <stdint.h>

#define NUM_FORMAT_STRINGS 4
#define FORMAT_STRING_SIZE 64

typedef struct format format;

struct format
{
  unsigned pos;
  char str[NUM_FORMAT_STRINGS][FORMAT_STRING_SIZE];
};

char *kissat_next_format_string (format *);

const char *kissat_format_bytes (format *, uint64_t bytes);
const char *kissat_format_count (format *, uint64_t);
const char *kissat_format_ordinal (format *, uint64_t);
const char *kissat_format_signs (format *, unsigned size, word);
const char *kissat_format_time (format *, uint64_t seconds);
const char *kissat_format_value (format *, bool boolean, int value);

#define FORMAT_BYTES(BYTES) \
  kissat_format_bytes (&solver->format, BYTES)

#define FORMAT_COUNT(WORD) \
  kissat_format_count (&solver->format, WORD)

#define FORMAT_ORDINAL(WORD) \
  kissat_format_ordinal (&solver->format, WORD)

#define FORMAT_SIGNS(SIZE, SIGNS) \
  kissat_format_signs (&solver->format, SIZE, SIGNS)

#define FORMAT_TIME(SECONDS) \
  kissat_format_time (&solver->format, SECONDS)

#define FORMAT_VALUE(BOOLEAN,VALUE) \
  kissat_format_value (&solver->format, BOOLEAN, VALUE)

#endif
