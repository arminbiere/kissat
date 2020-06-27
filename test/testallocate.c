#include "test.h"

#include "../src/allocate.h"
#include "../src/error.h"

#include <string.h>

static void
test_allocate_basic (void)
{
  DECLARE_AND_INIT_SOLVER (solver);

  int *p = kissat_malloc (solver, 1 << 30);
  assume (kissat_aligned_pointer (p));
  kissat_free (solver, p, 1 << 30);
#ifndef NMETRICS
  assert (!solver->statistics.allocated_current);
  assert (solver->statistics.allocated_max == 1u << 30);
#endif
  p = kissat_calloc (solver, 1 << 28, 4);
  assume (kissat_aligned_pointer (p));
  for (unsigned i = 0; i < 28; i++)
    assert (!p[1u << i]);
  kissat_dealloc (solver, p, 1 << 28, 4);
#ifndef NMETRICS
  assert (!solver->statistics.allocated_current);
  assert (solver->statistics.allocated_max == 1u << 30);
#endif
  char *s = kissat_strdup (solver, "test");
  assume (kissat_aligned_pointer (s));
  assert (!strcmp (s, "test"));
#ifndef NMETRICS
  assert (solver->statistics.allocated_current == 5);
#endif
  kissat_delstr (solver, s);
#ifndef NMETRICS
  assert (!solver->statistics.allocated_current);
#endif
}

static void
test_allocate_coverage (void)
{
  DECLARE_AND_INIT_SOLVER (solver);
  void *p = kissat_nrealloc (solver, 0, 0, 0, 0);
  assert (!p);
  p = kissat_realloc (solver, 0, 0, 0);
  assert (!p);
}

#ifndef ASAN

#include <setjmp.h>

static jmp_buf jump_buffer;

static void
abort_call_back (void)
{
  longjmp (jump_buffer, 42);
}

#define ALLOCATION_ERROR(CALL) \
do { \
  kissat_call_function_instead_of_abort (abort_call_back); \
  int val = setjmp (jump_buffer); \
  if (val) \
    { \
      kissat_call_function_instead_of_abort (0); \
      if (val != 42) \
	FATAL ("expected '42' as result of 'setjmp' for '" #CALL "'"); \
    } \
  else \
    { \
      CALL; \
      kissat_call_function_instead_of_abort (0); \
      FATAL ("long jump not taken in '" #CALL "'"); \
    } \
} while (0)

static void
test_allocate_error (void)
{
  DECLARE_AND_INIT_SOLVER (solver);
  ALLOCATION_ERROR (kissat_malloc (solver, MAX_SIZE_T));
  ALLOCATION_ERROR (kissat_calloc (solver, MAX_SIZE_T, MAX_SIZE_T));
  ALLOCATION_ERROR (kissat_calloc (solver, 1, MAX_SIZE_T));
  ALLOCATION_ERROR (kissat_calloc (solver, MAX_SIZE_T, 1));
  ALLOCATION_ERROR (kissat_realloc (solver, 0, 0, MAX_SIZE_T));
  ALLOCATION_ERROR (kissat_nrealloc (solver, 0, 0, MAX_SIZE_T, MAX_SIZE_T));
  ALLOCATION_ERROR (kissat_dealloc (solver, 0, MAX_SIZE_T, MAX_SIZE_T));
}

#endif

void
tissat_schedule_allocate (void)
{
  SCHEDULE_FUNCTION (test_allocate_basic);
  SCHEDULE_FUNCTION (test_allocate_coverage);
#ifndef ASAN
  SCHEDULE_FUNCTION (test_allocate_error);
#endif
}
