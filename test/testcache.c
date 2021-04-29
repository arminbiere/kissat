#include "../src/allocate.h"
#include "../src/cache.h"
#include "../src/nonces.h"
#include "../src/stack.h"

#include "test.h"

static void
test_cache_manual (void)
{
  DECLARE_AND_INIT_SOLVER (solver);
  bits *bits = kissat_lookup_cache (solver);
  assert (!bits);
  const unsigned vars = solver->vars = 8;
  CALLOC (solver->phases.saved, VARS);
  value *saved = solver->phases.saved;
  saved[0] = 1;
  kissat_insert_cache (solver, 3);
  bits = kissat_lookup_cache (solver);
  assert (bits);
  assert (kissat_get_bit (bits, vars, 0));
  for (unsigned i = 1; i < 8; i++)
    assert (!kissat_get_bit (bits, vars, i));
  memset (saved, 1, VARS);
  kissat_insert_cache (solver, 2);
  bits = kissat_lookup_cache (solver);
  assert (bits);
  for (unsigned i = 0; i < 8; i++)
    assert (kissat_get_bit (bits, vars, i));
  memset (saved, 0xff, VARS);
  kissat_insert_cache (solver, 1);
  saved[0] = 1;
  kissat_insert_cache (solver, 1);
  saved[3] = 1;
  kissat_insert_cache (solver, 1);
  saved[2] = 1;
  kissat_insert_cache (solver, 1);
  kissat_invalidate_cache (solver);
  kissat_insert_cache (solver, 1);
  kissat_release_cache (solver);
  RELEASE_STACK (solver->nonces);
  DEALLOC (solver->phases.saved, VARS);
}

#define RANDOM_IDX \
kissat_pick_random (&solver->random, 0, vars)

static void
test_cache_random (void)
{
  DECLARE_AND_INIT_SOLVER (solver);
  unsigned vars = 1;
  srand (42);
  for (unsigned i = 0; i < 10; i++)
    {
      printf ("%u variables\n", vars), fflush (stdout);
      solver->vars = vars;
      CALLOC (solver->phases.saved, VARS);
      value *saved = solver->phases.saved;
      memset (saved, 1, vars);
      kissat_invalidate_cache (solver);
      (void) kissat_lookup_cache (solver);
      for (unsigned j = 0; j < 10; j++)
	{
	  printf ("round %u\n", j), fflush (stdout);
	  for (unsigned k = 0; k < (vars + 3) / 3; k++)
	    saved[RANDOM_IDX] = 1;
	  for (unsigned k = 0; k < (vars + 3) / 3; k++)
	    saved[RANDOM_IDX] = -1;
	  for (unsigned k = 0; k < (vars + 1) / 2; k++)
	    saved[RANDOM_IDX] *= -1;

	  const unsigned unsatisfied =
	    kissat_pick_random (&solver->random, 0, i);
	  bool inserted = kissat_insert_cache (solver, unsatisfied);
	  const bits *bits = kissat_lookup_cache (solver);
	  if (!inserted)
	    continue;
	  if (SIZE_STACK (solver->cache.lines) > 1)
	    continue;
	  for (unsigned idx = 0; idx < vars; idx++)
	    {
	      bool bit = kissat_get_bit (bits, vars, idx);
	      value value = saved[idx];
	      if (bit)
		assert (value > 0);
	      else
		assert (value < 0);
	    }
	}
      DEALLOC (solver->phases.saved, VARS);
      vars = (2 + (i & 1)) * vars;
    }
  kissat_release_cache (solver);
  RELEASE_STACK (solver->nonces);
}

void
tissat_schedule_cache (void)
{
  SCHEDULE_FUNCTION (test_cache_manual);
  SCHEDULE_FUNCTION (test_cache_random);
}
