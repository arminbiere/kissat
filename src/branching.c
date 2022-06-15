#include "internal.h"
#include "print.h"

#ifndef QUIET

static const char *
branching_name (kissat * solver, unsigned branching)
{
#ifdef NOPTIONS
  (void) solver;
#endif
  return branching ? "CHB" : GET_OPTION (acids) ? "ACIDS" : "VSIDS";
}

#endif

void
kissat_init_branching (kissat * solver)
{
  if (!GET_OPTION (bump))
    return;

  solver->branching = GET_OPTION (chb) == 2;
  kissat_very_verbose (solver,
		       "will start with %s branching heuristics",
		       branching_name (solver, solver->branching));

  if (GET_OPTION (chb))
    {
      const int emachb = GET_OPTION (emachb);
      solver->alphachb = 1.0 / emachb;
      kissat_very_verbose (solver, "CHB branching alpha %g (window %d)",
			   solver->alphachb, emachb);
    }
}

bool
kissat_toggle_branching (kissat * solver)
{
  assert (solver->stable);
  if (!GET_OPTION (bump))
    return false;
  if (GET_OPTION (chb) != 1)
    return false;
#ifndef QUIET
  const unsigned current = solver->branching;
  const unsigned next = solver->branching = !current;
  kissat_extremely_verbose (solver,
			    "switching from %s branching heuristic to %s",
			    branching_name (solver, current),
			    branching_name (solver, next));
#else
  (void) solver;
#endif
  return true;
}
