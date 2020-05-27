#include "test.h"

#include <string.h>
#include <strings.h>

#include "../src/arena.h"
#include "../src/vector.h"

static void
test_format (void)
{
  format format;

  memset (&format, 0, sizeof format);

  (void) kissat_next_format_string (&format);	// coverage ...

#define FORMAT(TYPE,EXPR,EXPECTED) \
do { \
  const char * RES = kissat_format_ ## TYPE (&format, EXPR); \
  if (strcmp (RES, EXPECTED)) \
    FATAL ("kissat_format_" #TYPE " (.., " #EXPR ") = \"%s\" but expected \"%s\"", RES, EXPECTED); \
  printf ("kissat_format_" #TYPE " (.., " #EXPR ") = \"%s\" as expected\n", RES); \
} while (0)

#define FORMAT_signs(SIZE,SIGNS,EXPECTED) \
do { \
  const char * RES = kissat_format_signs (&format, SIZE, SIGNS); \
  if (strcmp (RES, EXPECTED)) \
    FATAL ("kissat_format_signs (.., " #SIZE ", " #SIGNS ") = \"%s\" but expected \"%s\"", RES, EXPECTED); \
  printf ("kissat_format_signs (.., " #SIZE ", " #SIGNS ") = \"%s\" as expected\n", RES); \
} while (0)

  FORMAT (count, 0, "0");
  FORMAT (count, 1, "1");
  FORMAT (count, 42, "42");
  FORMAT (count, 64, "64");
  FORMAT (count, 128, "2^7");
  FORMAT (count, 1024, "2^10");
  FORMAT (count, 1073741824, "2^30");

  char expected[80];

  sprintf (expected, "2^%u", LD_MAX_ARENA);
  FORMAT (count, MAX_ARENA, expected);

  sprintf (expected, "2^%u", LD_MAX_VECTORS);
  FORMAT (count, MAX_VECTORS, expected);

  FORMAT (count, 10, "10");
  FORMAT (count, 100, "100");
  FORMAT (count, 1e3, "1e3");
  FORMAT (count, 1e4, "1e4");
  FORMAT (count, 1e6, "1e6");
  FORMAT (count, 1e9, "1e9");

  FORMAT (bytes, 999, "999 bytes");
  FORMAT (bytes, 2050, "2050 bytes (2 KB)");
  FORMAT (bytes, 2150400, "2150400 bytes (2 MB)");
  FORMAT (bytes, 175000000, "175000000 bytes (167 MB)");
  FORMAT (bytes, 4000000000u, "4000000000 bytes (4 GB)");

  FORMAT (time, 0, "0s");
  FORMAT (time, 1, "1s");
  FORMAT (time, 61, "1m 1s");
  FORMAT (time, 3600, "1h");
  FORMAT (time, 3601, "1h 1s");
  FORMAT (time, 3661, "1h 1m 1s");
  FORMAT (time, 86400, "1d");
  FORMAT (time, 86405, "1d 5s");
  FORMAT (time, 86476, "1d 1m 16s");
  FORMAT (time, 108000, "1d 6h");
  FORMAT (time, 108001, "1d 6h 1s");
  FORMAT (time, 108180, "1d 6h 3m");
  FORMAT (time, 108127, "1d 6h 2m 7s");
  FORMAT (time, (uint64_t) 86400 * 30, "30d");
  FORMAT (time, (uint64_t) 86400 * 365, "365d");

  FORMAT_signs (0, 0, "");
  FORMAT_signs (1, 0, "0");
  FORMAT_signs (4, 0, "0000");
  FORMAT_signs (1, 1, "1");
  FORMAT_signs (4, 1, "1000");
  FORMAT_signs (4, 2, "0100");
  FORMAT_signs (4, 4, "0010");
  FORMAT_signs (4, 8, "0001");
  FORMAT_signs (4, 3, "1100");
  FORMAT_signs (4, 9, "1001");
  FORMAT_signs (4, 6, "0110");
  FORMAT_signs (4, 12, "0011");
  FORMAT_signs (8, 255, "11111111");

  FORMAT (ordinal, 0, "0th");
  FORMAT (ordinal, 1, "1st");
  FORMAT (ordinal, 2, "2nd");
  FORMAT (ordinal, 3, "3rd");
  FORMAT (ordinal, 4, "4th");
  FORMAT (ordinal, 9, "9th");
  FORMAT (ordinal, 10, "10th");
  FORMAT (ordinal, 11, "11th");
  FORMAT (ordinal, 21, "21st");
  FORMAT (ordinal, 22, "22nd");
  FORMAT (ordinal, 23, "23rd");
  FORMAT (ordinal, 24, "24th");
  FORMAT (ordinal, 2011, "2011th");
  FORMAT (ordinal, 2021, "2021st");

#undef FORMAT

#define FORMAT(EXPR,BOOLEAN,EXPECTED) \
do { \
  const char * res = kissat_format_value (&format, BOOLEAN, EXPR); \
  if (strcmp (res, EXPECTED)) \
    FATAL ("kissat_format_value (.., " #BOOLEAN ", " #EXPR ") = \"%s\" but expected \"%s\"", res, EXPECTED); \
  printf ("kissat_format_value (.., " #BOOLEAN ", " #EXPR ") = \"%s\" as expected\n", res); \
} while (0)

  FORMAT (0, true, "false");
  FORMAT (1, true, "true");
  FORMAT (2, true, "true");
  FORMAT (-1, true, "true");

  FORMAT (0, false, "0");
  FORMAT (1, false, "1");
  FORMAT (INT_MIN, false, "INT_MIN");
  FORMAT (INT_MAX, false, "INT_MAX");
  FORMAT (1e5, false, "1e5");
  FORMAT (-1e3, false, "-1e3");
  FORMAT (512, false, "2^9");
  FORMAT (-512, false, "-2^9");

#undef FORMAT
}

void
tissat_schedule_format (void)
{
  SCHEDULE_FUNCTION (test_format);
}
