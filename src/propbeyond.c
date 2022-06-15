#include "fastassign.h"
#include "propbeyond.h"
#include "trail.h"

#define PROPAGATE_LITERAL propagate_literal_beyond_conflicts
#define CONTINUE_PROPAGATING_AFTER_CONFLICT
#define PROPAGATION_TYPE "beyond conflict"

#include "proplit.h"

static void
propagate_literals_beyond_conflicts (kissat * solver)
{
  unsigned *propagate = solver->propagate;
  while (propagate != END_ARRAY (solver->trail))
    (void) propagate_literal_beyond_conflicts (solver, *propagate++);
  solver->propagate = propagate;
}

void
kissat_propagate_beyond_conflicts (kissat * solver)
{
  assert (!solver->probing);
  assert (solver->watching);
  assert (!solver->inconsistent);

  START (propagate);

  solver->ticks = 0;
  const unsigned *saved_propagate = solver->propagate;
  propagate_literals_beyond_conflicts (solver);
  kissat_update_search_propagation_statistics (solver, saved_propagate);

  STOP (propagate);
}
