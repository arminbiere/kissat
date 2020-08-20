#include "analyze.h"
#include "backtrack.h"
#include "bump.h"
#include "deduce.h"
#include "inline.h"
#include "learn.h"
#include "minimize.h"

#include <inttypes.h>

#define INVALID_LEVEL UINT_MAX

static bool
one_literal_on_conflict_level (kissat * solver,
			       clause * conflict,
			       unsigned *conflict_level_ptr)
{
  assert (conflict);
  assert (conflict->size > 1);

  unsigned conflict_level = INVALID_LEVEL;
  unsigned literals_on_conflict_level = 0;
  unsigned forced_lit = INVALID_LIT;

  assigned *all_assigned = solver->assigned;

  unsigned *lits = conflict->lits;
  const unsigned conflict_size = conflict->size;
  const unsigned *end_of_lits = lits + conflict_size;

  for (const unsigned *p = lits; p != end_of_lits; p++)
    {
      const unsigned lit = *p;
      assert (VALUE (lit) < 0);
      const unsigned idx = IDX (lit);
      const unsigned level = all_assigned[idx].level;
      if (conflict_level == level)
	{
	  if (++literals_on_conflict_level > 1 && level == solver->level)
	    break;
	}
      else if (conflict_level == INVALID_LEVEL || conflict_level < level)
	{
	  forced_lit = lit;
	  conflict_level = level;
	  literals_on_conflict_level = 1;
	}
    }
  assert (conflict_level != INVALID_LEVEL);
  assert (literals_on_conflict_level);

  LOG ("found %u literals on conflict level %u",
       literals_on_conflict_level, conflict_level);
  *conflict_level_ptr = conflict_level;

  if (!conflict_level)
    {
      solver->inconsistent = true;
      LOG ("learned empty clause from conflict at conflict level zero");
      CHECK_AND_ADD_EMPTY ();
      ADD_EMPTY_TO_PROOF ();
      return false;
    }

  if (conflict_level < solver->level)
    {
      LOG ("forced backtracking due to conflict level %u < level %u",
	   conflict_level, solver->level);
      kissat_backtrack (solver, conflict_level);
    }

  if (conflict_size > 2)
    {
      for (unsigned i = 0; i < 2; i++)
	{
	  const unsigned lit = lits[i];
	  const unsigned lit_idx = IDX (lit);
	  unsigned highest_position = i;
	  unsigned highest_literal = lit;
	  unsigned highest_level = all_assigned[lit_idx].level;
	  for (unsigned j = i + 1; j < conflict_size; j++)
	    {
	      const unsigned other = lits[j];
	      const unsigned other_idx = IDX (other);
	      const unsigned level = all_assigned[other_idx].level;
	      if (highest_level >= level)
		continue;
	      highest_literal = other;
	      highest_position = j;
	      highest_level = level;
	      if (highest_level == conflict_level)
		break;
	    }
	  if (highest_position == i)
	    continue;
	  reference ref = INVALID_REF;
	  if (highest_position > 1)
	    {
	      ref = kissat_reference_clause (solver, conflict);
	      kissat_unwatch_blocking (solver, lit, ref);
	    }
	  lits[highest_position] = lit;
	  lits[i] = highest_literal;
	  if (highest_position > 1)
	    kissat_watch_blocking (solver, lits[i], lits[!i], ref);
	}
    }

  if (literals_on_conflict_level > 1)
    return false;

  assert (literals_on_conflict_level == 1);
  assert (forced_lit != INVALID_LIT);

  LOG ("reusing conflict as driving clause of %s", LOGLIT (forced_lit));
  kissat_backtrack (solver, solver->level - 1);
  if (conflict_size == 2)
    {
      assert (conflict == &solver->conflict);
      const unsigned other = lits[0] ^ lits[1] ^ forced_lit;
      kissat_assign_binary (solver, conflict->redundant, forced_lit, other);
    }
  else
    {
      const reference ref = kissat_reference_clause (solver, conflict);
      kissat_assign_reference (solver, forced_lit, ref, conflict);
    }

  return true;
}

static void
mark_literal_as_analyzed (kissat * solver, assigned * all_assigned,
			  unsigned lit, const char *type)
{
  const unsigned idx = IDX (lit);
  assigned *a = all_assigned + idx;
  if (a->analyzed == ANALYZED)
    return;
  a->analyzed = ANALYZED;
  LOG ("marking %s literal %s as analyzed", type, LOGLIT (lit));
  PUSH_STACK (solver->analyzed, idx);
  (void) type;
}

static inline void
analyze_reason_side_literals (kissat * solver)
{
  assert (!solver->probing);
  if (!GET_OPTION (bumpreasons))
    return;
  assigned *all_assigned = solver->assigned;
  for (all_stack (unsigned, lit, solver->clause.lits))
      all_assigned[IDX (lit)].analyzed = ANALYZED;
  word *arena = BEGIN_STACK (solver->arena);
  for (all_stack (unsigned, lit, solver->clause.lits))
    {
      const unsigned idx = IDX (lit);
      assigned *a = all_assigned + idx;
      assert (a->level > 0);
      assert (a->reason != UNIT);
      if (a->reason == DECISION)
	continue;
      if (a->binary)
	mark_literal_as_analyzed (solver, all_assigned, a->reason,
				  "reason side");
      else
	{
	  assert (a->reason < SIZE_STACK (solver->arena));
	  clause *c = (clause *) (arena + a->reason);
	  for (all_literals_in_clause (lit, c))
	    if (IDX (lit) != idx)
	      mark_literal_as_analyzed (solver, all_assigned, lit,
					"reason side");
	}
    }
}

static void
reset_levels (kissat * solver)
{
  LOG ("reset %zu marked levels", SIZE_STACK (solver->levels));
  frame *frames = BEGIN_STACK (solver->frames);
  for (all_stack (unsigned, level, solver->levels))
    {
      assert (level < SIZE_STACK (solver->frames));
      frame *f = frames + level;
      assert (f->used > 0);
      f->used = 0;
    }
  CLEAR_STACK (solver->levels);
}

static void
reset_markings (kissat * solver)
{
  LOG ("unmarking %zu analyzed variables", SIZE_STACK (solver->analyzed));
  assigned *all_assigned = solver->assigned;
  for (all_stack (unsigned, idx, solver->analyzed))
      all_assigned[idx].analyzed = 0;
}

static void
reset_analyze (kissat * solver)
{
  LOG ("reset %zu analyzed variables", SIZE_STACK (solver->analyzed));
  CLEAR_STACK (solver->analyzed);

  LOG ("reset %zu learned literals", SIZE_STACK (solver->clause.lits));
  CLEAR_STACK (solver->clause.lits);
}

static void
update_trail_average (kissat * solver)
{
  assert (!solver->probing);
  const unsigned size = SIZE_STACK (solver->trail);
  const unsigned assigned = size - solver->unflushed;
  const unsigned active = solver->active;
  const double filled = kissat_percent (assigned, active);
  LOG ("trail filled %.0f%% (size %u, unflushed %u, active %u)",
       filled, size, solver->unflushed, active);
  UPDATE (trail, filled);
}

int
kissat_analyze (kissat * solver, clause * conflict)
{
  assert (!solver->inconsistent);
  START (analyze);
  if (!solver->probing)
    {
      update_trail_average (solver);
      UPDATE (level, solver->level);
    }
  int res;
  do
    {
      LOGCLS (conflict, "analyzing conflict %" PRIu64, CONFLICTS);
      unsigned conflict_level;
      if (one_literal_on_conflict_level (solver, conflict, &conflict_level))
	res = 1;
      else if (!conflict_level)
	res = -1;
      else if ((conflict = kissat_deduce_first_uip_clause (solver, conflict)))
	{
	  reset_markings (solver);
	  reset_analyze (solver);
	  reset_levels (solver);
	  res = 0;
	}
      else
	{
	  kissat_minimize_clause (solver);
	  if (!solver->probing)
	    analyze_reason_side_literals (solver);
	  reset_markings (solver);
	  kissat_learn_clause (solver);
	  if (!solver->probing)
	    kissat_bump_variables (solver);
	  reset_analyze (solver);
	  reset_levels (solver);
	  res = 1;
	}
    }
  while (!res);
  STOP (analyze);
  return res > 0 ? 0 : 20;
}
