#ifndef _tissat_h_INCLUDED
#define _tissat_h_INCLUDED

#include "../src/inline.h"
#include "../src/print.h"

#include "testapplication.h"
#include "testdivert.h"
#include "testmessages.h"
#include "testscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

extern bool tissat_big;
extern bool tissat_found_test_directory;
extern bool tissat_sequential;
extern bool tissat_progress;

#ifndef NPROOFS
extern bool tissat_found_drabt;
extern bool tissat_found_drat_trim;
#endif

#ifdef _POSIX_C_SOURCE
extern bool tissat_found_bzip2;
extern bool tissat_found_gzip;
extern bool tissat_found_lzma;
extern bool tissat_found_xz;
extern bool tissat_found_7z;
#endif

extern const char *tissat_root;

#define tissat_assert(COND) \
( \
  (COND) ? \
    (void) 0 \
  : \
  \
    ( \
      tissat_restore_stdout_and_stderr (), \
      printf ("tissat: %s:%ld: %s: Assertion `%s' failed.\n", \
	__FILE__, (long) __LINE__, __func__, #COND), \
      abort (), \
      (void) 0 \
    ) \
)

#define tissat_assume(COND) \
( \
  (COND) ? \
    (void) 0 \
  : \
  \
    ( \
      tissat_restore_stdout_and_stderr (), \
      tissat_warning ("tissat: %s:%ld: %s: Assumption `%s' failed.\n", \
	__FILE__, (long) __LINE__, __func__, #COND), \
      tissat_divert_stdout_and_stderr_to_dev_null (), \
      tissat_warnings++, \
      (void) 0 \
    ) \
)

#ifdef assert
#undef assert
#endif

#define assume tissat_assume
#define assert tissat_assert

#define FATAL(...) \
do { \
  fflush (stdout); \
  tissat_restore_stdout_and_stderr (); \
  tissat_fatal (__VA_ARGS__); \
} while (0)

void tissat_init_solver (struct kissat *);

#define DECLARE_AND_INIT_SOLVER(SOLVER) \
  kissat dummy_solver, *solver = &dummy_solver; \
  memset (&dummy_solver, 0, sizeof dummy_solver); \
  tissat_init_solver (solver)

#endif
