#include "allocate.h"
#include "backtrack.h"
#include "collect.h"
#include "dense.h"
#include "eliminate.h"
#include "forward.h"
#include "inline.h"
#include "kitten.h"
#include "propdense.h"
#include "print.h"
#include "report.h"
#include "resolve.h"
#include "terminate.h"
#include "trail.h"
#include "weaken.h"

#include <inttypes.h>
#include <math.h>

static uint64_t
eliminate_adjustment (kissat * solver)
{
  return 2 * CLAUSES + NLOGN (1 + solver->active);
}

bool
kissat_eliminating (kissat * solver)
{
  if (!solver->enabled.eliminate)
    return false;
  statistics *statistics = &solver->statistics;
  if (!statistics->clauses_irredundant)
    return false;
  if (solver->waiting.eliminate.reduce > statistics->reductions)
    return false;
  limits *limits = &solver->limits;
  if (limits->eliminate.conflicts > statistics->conflicts)
    return false;
  if (limits->eliminate.variables.added < statistics->variables_added)
    return true;
  if (limits->eliminate.variables.removed < statistics->variables_removed)
    return true;
  return false;
}

void
kissat_eliminate_binary (kissat * solver, unsigned lit, unsigned other)
{
  kissat_disconnect_binary (solver, other, lit);
  kissat_delete_binary (solver, false, false, lit, other);
}

void
kissat_flush_units_while_connected (kissat * solver)
{
  const unsigned *propagate = solver->propagate;
  const unsigned *end_trail = END_ARRAY (solver->trail);
  assert (propagate <= end_trail);
  const size_t units = end_trail - propagate;
  if (!units)
    return;
#ifdef LOGGING
  LOG ("propagating and flushing %zu units", units);
#endif
  if (!kissat_dense_propagate (solver))
    return;
  LOG ("marking and flushing unit satisfied clauses");

  end_trail = END_ARRAY (solver->trail);
  while (propagate != end_trail)
    {
      const unsigned unit = *propagate++;
      watches *unit_watches = &WATCHES (unit);
      watch *begin = BEGIN_WATCHES (*unit_watches), *q = begin;
      const watch *const end = END_WATCHES (*unit_watches), *p = q;
      if (begin == end)
	continue;
      LOG ("marking %s satisfied clauses as garbage", LOGLIT (unit));
      while (p != end)
	{
	  const watch watch = *q++ = *p++;
	  if (watch.type.binary)
	    continue;
	  const reference ref = watch.large.ref;
	  clause *c = kissat_dereference_clause (solver, ref);
	  if (!c->garbage)
	    kissat_mark_clause_as_garbage (solver, c);
	  assert (c->garbage);
	  q--;
	}
      assert (q <= end);
      size_t flushed = end - q;
      if (!flushed)
	continue;
      LOG ("flushing %zu references satisfied by %s", flushed, LOGLIT (unit));
      SET_END_OF_WATCHES (*unit_watches, q);
    }
}

static void
connect_resolvents (kissat * solver)
{
  const value *const values = solver->values;
  assert (EMPTY_STACK (solver->clause));
  bool satisfied = false;
  uint64_t added = 0;
  for (all_stack (unsigned, other, solver->resolvents))
    {
      if (other == INVALID_LIT)
	{
	  if (satisfied)
	    satisfied = false;
	  else
	    {
	      LOGTMP ("temporary resolvent");
	      const size_t size = SIZE_STACK (solver->clause);
	      if (!size)
		{
		  assert (!solver->inconsistent);
		  LOG ("resolved empty clause");
		  CHECK_AND_ADD_EMPTY ();
		  ADD_EMPTY_TO_PROOF ();
		  solver->inconsistent = true;
		  break;
		}
	      else if (size == 1)
		{
		  const unsigned unit = PEEK_STACK (solver->clause, 0);
		  LOG ("resolved unit clause %s", LOGLIT (unit));
		  kissat_learned_unit (solver, unit);
		}
	      else
		{
		  assert (size > 1);
		  (void) kissat_new_irredundant_clause (solver);
		  added++;
		}
	    }
	  CLEAR_STACK (solver->clause);
	}
      else if (!satisfied)
	{
	  const value value = values[other];
	  if (value > 0)
	    {
	      LOGTMP ("now %s satisfied resolvent", LOGLIT (other));
	      satisfied = true;
	    }
	  else if (value < 0)
	    LOG2 ("dropping now falsified literal %s", LOGLIT (other));
	  else
	    PUSH_STACK (solver->clause, other);
	}
    }
  LOG ("added %" PRIu64 " new clauses", added);
  CLEAR_STACK (solver->resolvents);
}

static void
weaken_clauses (kissat * solver, unsigned lit)
{
  const unsigned not_lit = NOT (lit);

  const value *const values = solver->values;
  assert (!values[lit]);

  watches *pos_watches = &WATCHES (lit);

  for (all_binary_large_watches (watch, *pos_watches))
    {
      if (watch.type.binary)
	{
	  const unsigned other = watch.binary.lit;
	  const value value = values[other];
	  if (value <= 0)
	    kissat_weaken_binary (solver, lit, other);
	  assert (!watch.binary.redundant);
	  kissat_eliminate_binary (solver, lit, other);
	}
      else
	{
	  const reference ref = watch.large.ref;
	  clause *c = kissat_dereference_clause (solver, ref);
	  if (c->garbage)
	    continue;
	  bool satisfied = false;
	  for (all_literals_in_clause (other, c))
	    {
	      const value value = values[other];
	      if (value <= 0)
		continue;
	      satisfied = true;
	      break;
	    }
	  if (!satisfied)
	    kissat_weaken_clause (solver, lit, c);
	  LOGCLS (c, "removing %s", LOGLIT (lit));
	  kissat_mark_clause_as_garbage (solver, c);
	}
    }
  RELEASE_WATCHES (*pos_watches);

  watches *neg_watches = &WATCHES (not_lit);

  bool optimize = !GET_OPTION (incremental);
  for (all_binary_large_watches (watch, *neg_watches))
    {
      if (watch.type.binary)
	{
	  const unsigned other = watch.binary.lit;
	  assert (!watch.binary.redundant);
	  const value value = values[other];
	  if (!optimize && value <= 0)
	    kissat_weaken_binary (solver, not_lit, other);
	  kissat_eliminate_binary (solver, not_lit, other);
	}
      else
	{
	  const reference ref = watch.large.ref;
	  clause *d = kissat_dereference_clause (solver, ref);
	  if (d->garbage)
	    continue;
	  bool satisfied = false;
	  for (all_literals_in_clause (other, d))
	    {
	      const value value = values[other];
	      if (value <= 0)
		continue;
	      satisfied = true;
	      break;
	    }
	  if (!optimize && !satisfied)
	    kissat_weaken_clause (solver, not_lit, d);
	  LOGCLS (d, "removing %s", LOGLIT (not_lit));
	  kissat_mark_clause_as_garbage (solver, d);
	}
    }
  if (optimize && !EMPTY_WATCHES (*neg_watches))
    kissat_weaken_unit (solver, not_lit);
  RELEASE_WATCHES (*neg_watches);

  kissat_flush_units_while_connected (solver);
}

static void
try_to_eliminate_all_variables_again (kissat * solver)
{
  LOG ("trying to elimination all variables again");
  flags *all_flags = solver->flags;
  for (all_variables (idx))
    {
      flags *flags = all_flags + idx;
      flags->eliminate = true;
    }
  solver->limits.eliminate.variables.removed = 0;
}

static void
set_next_elimination_bound (kissat * solver, bool complete)
{
  const unsigned max_bound = GET_OPTION (eliminatebound);
  const unsigned current_bound = solver->bounds.eliminate.additional_clauses;
  assert (current_bound <= max_bound);

  if (complete)
    {
      if (current_bound == max_bound)
	{
	  kissat_phase (solver, "eliminate", GET (eliminations),
			"completed maximum elimination bound %u",
			current_bound);
	  limits *limits = &solver->limits;
	  statistics *statistics = &solver->statistics;
	  limits->eliminate.variables.added = statistics->variables_added;
	  limits->eliminate.variables.removed = statistics->variables_removed;
#ifndef QUIET
	  bool first = !solver->bounds.eliminate.max_bound_completed++;
	  REPORT (!first, first ? '!' : ':');
#endif
	}
      else
	{
	  const unsigned next_bound =
	    !current_bound ? 1 : MIN (2 * current_bound, max_bound);
	  kissat_phase (solver, "eliminate", GET (eliminations),
			"completed elimination bound %u next %u",
			current_bound, next_bound);
	  solver->bounds.eliminate.additional_clauses = next_bound;
	  try_to_eliminate_all_variables_again (solver);
	  REPORT (0, '^');
	}
    }
  else
    kissat_phase (solver, "eliminate", GET (eliminations),
		  "incomplete elimination bound %u", current_bound);
}

static bool
can_eliminate_variable (kissat * solver, unsigned idx)
{
  flags *flags = FLAGS (idx);

  if (!flags->active)
    return false;
  if (!flags->eliminate)
    return false;

  return true;
}

static bool
eliminate_variable (kissat * solver, unsigned idx)
{
  LOG ("next elimination candidate %s", LOGVAR (idx));

  assert (!solver->inconsistent);
  assert (can_eliminate_variable (solver, idx));

  LOG ("marking %s as not removed", LOGVAR (idx));
  FLAGS (idx)->eliminate = false;

  unsigned lit;
  if (!kissat_generate_resolvents (solver, idx, &lit))
    return false;
  connect_resolvents (solver);
  if (!solver->inconsistent)
    weaken_clauses (solver, lit);
  INC (eliminated);
  kissat_mark_eliminated_variable (solver, idx);
  if (solver->gate_eliminated)
    {
      INC (gates_eliminated);
#ifdef METRICS
      assert (*solver->gate_eliminated < UINT64_MAX);
      *solver->gate_eliminated += 1;
#endif
    }
  return true;
}

static void
eliminate_variables (kissat * solver)
{
  kissat_very_verbose (solver,
		       "trying to eliminate variables with bound %u",
		       solver->bounds.eliminate.additional_clauses);
  assert (!solver->inconsistent);
#ifndef QUIET
  unsigned before = solver->active;
#endif
  unsigned eliminated = 0;
  uint64_t tried = 0;

  SET_EFFORT_LIMIT (resolution_limit,
		    eliminate, eliminate_resolutions,
		    eliminate_adjustment (solver));

  bool complete;
  int round = 0;

  const bool forward = GET_OPTION (forward);

  for (;;)
    {
      round++;
      LOG ("starting new elimination round %d", round);

      if (forward)
	{
	  unsigned *propagate = solver->propagate;
	  complete = kissat_forward_subsume_during_elimination (solver);
	  if (solver->inconsistent)
	    break;
	  kissat_flush_large_connected (solver);
	  kissat_connect_irredundant_large_clauses (solver);
	  solver->propagate = propagate;
	  kissat_flush_units_while_connected (solver);
	  if (solver->inconsistent)
	    break;
	}
      else
	{
	  kissat_connect_irredundant_large_clauses (solver);
	  complete = true;
	}

      unsigned successful = 0;

      for (unsigned idx = 0; idx != solver->vars; idx++)
	{
	  if (TERMINATED (eliminate_terminated_1))
	    {
	      complete = false;
	      break;
	    }
	  if (!can_eliminate_variable (solver, idx))
	    continue;
	  if (solver->statistics.eliminate_resolutions > resolution_limit)
	    {
	      kissat_extremely_verbose (solver,
					"eliminate round %u hits "
					"resolution limit %"
					PRIu64 " at %" PRIu64 " resolutions",
					round, resolution_limit,
					solver->
					statistics.eliminate_resolutions);
	      complete = false;
	      break;
	    }
	  tried++;
	  if (eliminate_variable (solver, idx))
	    successful++;
	  if (solver->inconsistent)
	    break;
	  kissat_flush_units_while_connected (solver);
	  if (solver->inconsistent)
	    break;
	}

      if (successful)
	{
	  complete = false;
	  eliminated += successful;
	}

      if (!solver->inconsistent)
	{
	  kissat_flush_large_connected (solver);
	  kissat_dense_collect (solver);
	}

      kissat_phase (solver, "eliminate", GET (eliminations),
		    "eliminated %u variables in round %u", successful, round);
      REPORT (!successful, 'e');

      if (solver->inconsistent)
	break;
      if (complete)
	break;
      if (round == GET_OPTION (eliminaterounds))
	break;
      if (solver->statistics.eliminate_resolutions > resolution_limit)
	break;
      if (TERMINATED (eliminate_terminated_2))
	break;
    }

#ifndef QUIET
  kissat_very_verbose (solver,
		       "eliminated %u variables %.0f%% of %" PRIu64 " tried",
		       eliminated, kissat_percent (eliminated, tried), tried);
  kissat_phase (solver, "eliminate", GET (eliminations),
		"eliminated %u variables %.0f%% out of %u in %d rounds",
		eliminated, kissat_percent (eliminated, before),
		before, round);
#endif
  if (!solver->inconsistent)
    set_next_elimination_bound (solver, complete);
}

static void
init_map_and_kitten (kissat * solver)
{
  if (!GET_OPTION (definitions))
    return;
  assert (!solver->kitten);
  solver->kitten = kitten_embedded (solver);
}

static void
reset_map_and_kitten (kissat * solver)
{
  if (solver->kitten)
    {
      kitten_release (solver->kitten);
      solver->kitten = 0;
    }
}

static void
eliminate (kissat * solver)
{
  kissat_backtrack_propagate_and_flush_trail (solver);
  assert (!solver->inconsistent);
  STOP_SEARCH_AND_START_SIMPLIFIER (eliminate);
  kissat_phase (solver, "eliminate", GET (eliminations),
		"elimination limit of %" PRIu64 " conflicts hit",
		solver->limits.eliminate.conflicts);
  init_map_and_kitten (solver);
  litwatches saved;
  INIT_STACK (saved);
  kissat_enter_dense_mode (solver, 0, &saved);
  eliminate_variables (solver);
  kissat_resume_sparse_mode (solver, true, 0, &saved);
  RELEASE_STACK (saved);
  reset_map_and_kitten (solver);
  kissat_check_statistics (solver);
  STOP_SIMPLIFIER_AND_RESUME_SEARCH (eliminate);
}

int
kissat_eliminate (kissat * solver)
{
  assert (!solver->inconsistent);
  INC (eliminations);
  eliminate (solver);
  UPDATE_CONFLICT_LIMIT (eliminate, eliminations, NLOG2N, true);
  solver->waiting.eliminate.reduce = solver->statistics.reductions + 1;
  solver->last.eliminate = solver->statistics.search_ticks;
  return solver->inconsistent ? 20 : 0;
}
