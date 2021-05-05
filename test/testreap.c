#include "../src/inlinereap.h"

#include "test.h"

static void
test_reap_random (void)
{
  srand (42);
  DECLARE_AND_INIT_SOLVER (solver);
  reap reap;
  kissat_init_reap (solver, &reap);
#define N 3000
#define M 1000
  for (unsigned i = 0; i < N; i++)
    {
      unsigned tmp = rand () % M;
      printf ("push %u\n", tmp);
      kissat_push_reap (solver, &reap, tmp);
    }
  unsigned last = 0, count = 2;
  while (!kissat_empty_reap (&reap))
    {
      unsigned popped = kissat_pop_reap (solver, &reap);
      printf ("pop %u\n", popped);
      assert (last <= popped);
      last = popped;
      if (count--)
	continue;
      unsigned tmp = last + (rand () % M);
      printf ("push %u\n", tmp);
      kissat_push_reap (solver, &reap, tmp);
      count = 1 + (rand () % 3);
    }
  kissat_release_reap (solver, &reap);
}

void
tissat_schedule_reap (void)
{
  SCHEDULE_FUNCTION (test_reap_random);
}
