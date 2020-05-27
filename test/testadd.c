#include "test.h"

static word full_clauses;

static void
add_full_clauses (kissat * solver, int *clause, int i, int n)
{
  assert (0 < i);
  assert (i <= n);
  for (int sign = -1; sign <= 1; sign += 2)
    {
      clause[i] = sign * i;
      if (i == n)
	{
	  for (int j = 1; j <= i; j++)
	    kissat_add (solver, clause[j]);
	  kissat_add (solver, 0);
	  full_clauses++;
	}
      else
	add_full_clauses (solver, clause, i + 1, n);
    }
}

static void
test_add (void)
{
  const int m = tissat_big ? 16 : 10;

  for (int n = 1; n < m; n++)
    {
      int clause[n + 1];
      full_clauses = 0;
      kissat *solver = kissat_init ();
      add_full_clauses (solver, clause, 1, n);
      size_t arena = CAPACITY_STACK (solver->arena) * sizeof (word);
      printf ("%d: arena %s clauses %s\n", n,
	      kissat_format_bytes (&solver->format, arena),
	      kissat_format_count (&solver->format, full_clauses));
      kissat_release (solver);
    }
}

void
tissat_schedule_add (void)
{
  SCHEDULE_FUNCTION (test_add);
}
