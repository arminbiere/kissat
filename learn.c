#include "backtrack.h"
#include "inline.h"
#include "learn.h"
#include "reluctant.h"

#include <inttypes.h>

static unsigned
determine_new_level (kissat * solver, unsigned jump)
{
  assert (solver->level);
  const unsigned back = solver->level - 1;
  assert (jump <= back);

  const unsigned delta = back - jump;
  const unsigned limit =
    GET_OPTION (chrono) ? (unsigned) GET_OPTION (chronolevels) : UINT_MAX;

  unsigned res;

  if (!delta)
    {
      res = jump;
      LOG ("using identical backtrack and jump level %u", res);
    }
  else if (delta > limit)
    {
      res = back;
      LOG ("backjumping over %u levels (%u - %u) considered inefficient",
	   delta, back, jump);
      LOG ("backtracking chronologically to backtrack level %u", res);
      INC (chronological);
    }
  else
    {
      res = jump;
      LOG ("backjumping over %u levels (%u - %u) considered efficient",
	   delta, back, jump);
      LOG ("backjumping non-chronologically to jump level %u", res);
    }

  return res;
}

static void
learn_unit (kissat * solver)
{
  const unsigned *lits = BEGIN_STACK (solver->clause.lits);
  const unsigned not_uip = lits[0];
  LOG ("learned unit clause %s triggers iteration", LOGLIT (not_uip));
  const unsigned new_level = determine_new_level (solver, 0);
  kissat_backtrack (solver, new_level);
  kissat_assign_unit (solver, not_uip);
  solver->iterating = true;
  CHECK_AND_ADD_UNIT (not_uip);
  ADD_UNIT_TO_PROOF (not_uip);
}

static void
learn_binary (kissat * solver)
{
  const unsigned *lits = BEGIN_STACK (solver->clause.lits);
  const unsigned not_uip = lits[0];
  const unsigned other = lits[1];
  const unsigned jump_level = LEVEL (other);
  const unsigned new_level = determine_new_level (solver, jump_level);
  kissat_backtrack (solver, new_level);
  const unsigned glue = SIZE_STACK (solver->levels);
  assert (glue == 1);
#ifndef NDEBUG
  const reference ref =
#endif
    kissat_new_redundant_clause (solver, glue);
  assert (ref == INVALID_REF);
  kissat_assign_binary (solver, true, not_uip, other);
  kissat_eager_subsume (solver);
}

static void
learn_reference (kissat * solver)
{
  assert (solver->level > 1);
  unsigned *lits = BEGIN_STACK (solver->clause.lits);
  const unsigned not_uip = lits[0];
  unsigned *q = lits + 1;
  unsigned jump_lit = *q;
  unsigned jump_level = LEVEL (jump_lit);
  const unsigned *end = END_STACK (solver->clause.lits);
  const unsigned backtrack_level = solver->level - 1;
  assigned *all_assigned = solver->assigned;
  for (unsigned *p = lits + 2; p != end; p++)
    {
      const unsigned lit = *p;
      const unsigned idx = IDX (lit);
      const unsigned level = all_assigned[idx].level;
      if (jump_level >= level)
	continue;
      jump_level = level;
      jump_lit = lit;
      q = p;
      if (level == backtrack_level)
	break;
    }
  *q = lits[1];
  lits[1] = jump_lit;
  const unsigned glue = SIZE_STACK (solver->levels);
  const reference ref = kissat_new_redundant_clause (solver, glue);
  assert (ref != INVALID_REF);
  clause *c = kissat_dereference_clause (solver, ref);
  c->used = 1 + (glue <= (unsigned) GET_OPTION (tier2));
  const unsigned new_level = determine_new_level (solver, jump_level);
  kissat_backtrack (solver, new_level);
  kissat_assign_reference (solver, not_uip, ref, c);
  kissat_eager_subsume (solver);
  kissat_push_clueue (&solver->clueue, ref);
}

void
kissat_learn_clause (kissat * solver)
{
  if (!solver->probing)
    INC (learned);
  if (solver->stable)
    kissat_tick_reluctant (&solver->reluctant);
  const unsigned glue = SIZE_STACK (solver->levels);
  const unsigned size = SIZE_STACK (solver->clause.lits);
  LOG ("learned[%" PRIu64 "] clause glue %u size %u",
       GET (learned), glue, size);
  if (!solver->probing)
    {
      ADD (literals_learned, size);
      UPDATE (size, size);
      UPDATE (fast_glue, glue);
      UPDATE (slow_glue, glue);
    }
  assert (size > 0);
  if (size == 1)
    learn_unit (solver);
  else if (size == 2)
    learn_binary (solver);
  else
    learn_reference (solver);
}
