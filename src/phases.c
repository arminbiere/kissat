#include "internal.h"
#include "logging.h"

void
kissat_save_target_phases (kissat * solver)
{
  LOG ("saving %u target values", LITS);
  const value *v = solver->values;
  for (all_phases (p))
    p->target = *v, v += 2;
  assert (v == solver->values + LITS);
}

void
kissat_clear_target_phases (kissat * solver)
{
  LOG ("clearing %u target values", LITS);
  const value *v = solver->values;
  for (all_phases (p))
    p->target = *v, v += 2;
  assert (v == solver->values + LITS);
}

void
kissat_save_best_phases (kissat * solver)
{
  LOG ("saving %u target values", LITS);
  const value *v = solver->values;
  for (all_phases (p))
    p->best = *v, v += 2;
  assert (v == solver->values + LITS);
}
