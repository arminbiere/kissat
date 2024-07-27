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
#include <string.h>

extern bool tissat_big;
extern bool tissat_found_test_directory;
extern bool tissat_sequential;
extern bool tissat_progress;

#ifndef NPROOFS
extern bool tissat_found_drabt;
extern bool tissat_found_drat_trim;
extern bool tissat_found_dpr_trim;
#endif

#if defined(_POSIX_C_SOURCE) || defined(__APPLE__)
extern bool tissat_found_bzip2;
extern bool tissat_found_gzip;
extern bool tissat_found_lzma;
extern bool tissat_found_xz;
extern bool tissat_found_7z;
#endif

extern const char *tissat_root;

#define tissat_assert(COND) \
  ((COND) ? (void) 0 : \
\
          (tissat_restore_stdout_and_stderr (), \
           printf ("tissat: %s:%ld: %s: Assertion `%s' failed.\n", \
                   __FILE__, (long) __LINE__, __func__, #COND), \
           abort (), (void) 0))

#define tissat_assume(COND) \
  ((COND) \
       ? (void) 0 \
       : \
\
       (tissat_restore_stdout_and_stderr (), \
        tissat_warning ("tissat: %s:%ld: %s: Assumption `%s' failed.\n", \
                        __FILE__, (long) __LINE__, __func__, #COND), \
        tissat_divert_stdout_and_stderr_to_dev_null (), tissat_warnings++, \
        (void) 0))

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

extern kissat kissat_test_dummy_solver;

#define DECLARE_AND_INIT_SOLVER(SOLVER) \
  kissat *solver = &kissat_test_dummy_solver; \
  memset (solver, 0, sizeof *solver); \
  tissat_init_solver (solver)

#endif
