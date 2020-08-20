#include "allocate.h"
#include "backtrack.h"
#include "backward.h"
#include "collect.h"
#include "dense.h"
#include "eliminate.h"
#include "forward.h"
#include "inline.h"
#include "propdense.h"
#include "print.h"
#include "report.h"
#include "resolve.h"
#include "terminate.h"
#include "trail.h"
#include "weaken.h"

#include <inttypes.h>
#include <math.h>

static bool
really_eliminate (kissat * solver)
{
  if (!GET_OPTION (really))
    return true;
  const uint64_t limit = 2 * CLAUSES;
  statistics *statistics = &solver->statistics;
  return limit < statistics->search_ticks + GET_OPTION (eliminatemineff);
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
  if (!really_eliminate (solver))
    return false;
  if (limits->eliminate.variables.added < statistics->variables_added)
    return true;
  if (limits->eliminate.variables.removed < statistics->variables_removed)
    return true;
  return false;
}

static inline void
update_after_adding_variable (kissat * solver, heap * schedule, unsigned idx)
{
  assert (schedule->size);
  kissat_update_variable_score (solver, schedule, idx);
}

static inline void
update_after_adding_stack (kissat * solver, unsigneds * stack)
{
  assert (!solver->probing);
  heap *schedule = &solver->schedule;
  if (!schedule->size)
    return;
  for (all_stack (unsigned, lit, *stack))
      update_after_adding_variable (solver, schedule, IDX (lit));
}

static inline void
update_after_removing_variable (kissat * solver, unsigned idx)
{
  assert (!solver->probing);
  heap *schedule = &solver->schedule;
  if (!schedule->size)
    return;
  kissat_update_variable_score (solver, schedule, idx);
  if (!kissat_heap_contains (schedule, idx))
    kissat_push_heap (solver, schedule, idx);
}

void
kissat_update_after_removing_variable (kissat * solver, unsigned idx)
{
  update_after_removing_variable (solver, idx);
}

static inline void
update_after_removing_clause (kissat * solver, clause * c, unsigned except)
{
  assert (c->garbage);
  for (all_literals_in_clause (lit, c))
    if (lit != except)
      update_after_removing_variable (solver, IDX (lit));
}

void
kissat_update_after_removing_clause (kissat * solver,
				     clause * c, unsigned except)
{
  update_after_removing_clause (solver, c, except);
}

void
kissat_eliminate_binary (kissat * solver, unsigned lit, unsigned other)
{
  kissat_disconnect_binary (solver, other, lit);
  kissat_delete_binary (solver, false, false, lit, other);
  update_after_removing_variable (solver, IDX (other));
}

void
kissat_eliminate_clause (kissat * solver, clause * c, unsigned lit)
{
  kissat_mark_clause_as_garbage (solver, c);
  update_after_removing_clause (solver, c, lit);
}

static size_t
schedule_variables (kissat * solver)
{
  LOG ("initializing variable schedule");
  assert (!solver->schedule.size);
  kissat_resize_heap (solver, &solver->schedule, solver->vars);

  flags *all_flags = solver->flags;

  for (all_variables (idx))
    {
      flags *flags = all_flags + idx;
      if (!flags->active)
	continue;
      if (!flags->eliminate)
	continue;
      LOG ("scheduling variable %u", idx);
      update_after_removing_variable (solver, idx);
    }
  size_t scheduled = kissat_size_heap (&solver->schedule);
#ifndef QUIET
  size_t active = solver->active;
  kissat_phase (solver, "eliminate", GET (eliminations),
		"scheduled %zu variables %.0f%%",
		scheduled, kissat_percent (scheduled, active));
#endif
  return scheduled;
}

void
kissat_flush_units_while_connected (kissat * solver)
{
  unsigned propagated = solver->propagated;
  size_t units = SIZE_STACK (solver->trail) - propagated;
  if (!units)
    return;
#ifdef LOGGING
  LOG ("propagating and flushing %zu units", units);
#endif
  if (!kissat_dense_propagate (solver,
			       NO_DENSE_PROPAGATION_LIMIT, INVALID_IDX))
    {
      assert (!solver->inconsistent);
      LOG ("inconsistent root propagation of resolved units");
      CHECK_AND_ADD_EMPTY ();
      ADD_EMPTY_TO_PROOF ();
      solver->inconsistent = true;
      return;
    }

  LOG ("marking and flushing unit satisfied clauses");
  const value *values = solver->values;
  while (propagated < solver->propagated)
    {
      const unsigned unit = PEEK_STACK (solver->trail, propagated);
      propagated++;
      assert (values[unit] > 0);
      watches *unit_watches = &WATCHES (unit);
      watch *begin = BEGIN_WATCHES (*unit_watches), *q = begin;
      const watch *end = END_WATCHES (*unit_watches), *p = q;
      if (begin == end)
	continue;
      LOG ("marking %s satisfied clauses as garbage", LOGLIT (unit));
      while (p != end)
	{
	  const watch watch = *q++ = *p++;
	  if (watch.type.binary)
	    {
	      const unsigned other = watch.binary.lit;
	      if (!values[other])
		update_after_removing_variable (solver, IDX (other));
	    }
	  else
	    {
	      const reference ref = watch.large.ref;
	      clause *c = kissat_dereference_clause (solver, ref);
	      if (!c->garbage)
		kissat_eliminate_clause (solver, c, unit);
	      assert (c->garbage);
	      q--;
	    }
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
  bool backward = GET_OPTION (backward);
  const value *values = solver->values;
  assert (EMPTY_STACK (solver->clause.lits));
  uint64_t added = 0;
  bool satisfied = false;
  for (all_stack (unsigned, other, solver->resolvents))
    {
      if (other == INVALID_LIT)
	{
	  if (satisfied)
	    satisfied = false;
	  else if (kissat_forward_subsume_temporary (solver))
	    LOGTMP ("temporary forward subsumed");
	  else
	    {
	      size_t size = SIZE_STACK (solver->clause.lits);
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
		  const unsigned unit = PEEK_STACK (solver->clause.lits, 0);
		  LOG ("resolved unit clause %s", LOGLIT (unit));
		  kissat_assign_unit (solver, unit);
		  CHECK_AND_ADD_UNIT (unit);
		  ADD_UNIT_TO_PROOF (unit);
		}
	      else
		{
		  assert (size > 1);
		  reference ref = kissat_new_irredundant_clause (solver);
		  update_after_adding_stack (solver, &solver->clause.lits);
		  added++;

		  if (backward)
		    backward =
		      kissat_backward_subsume_temporary (solver, ref);
		}
	    }
	  CLEAR_STACK (solver->clause.lits);
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
	    PUSH_STACK (solver->clause.lits, other);
	}
    }
  LOG ("added %" PRIu64 " new clauses", added);
  CLEAR_STACK (solver->resolvents);
}

static void
weaken_clauses (kissat * solver, unsigned lit)
{
  const unsigned not_lit = NOT (lit);

  const value *values = solver->values;
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
	  kissat_eliminate_clause (solver, c, lit);
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
	  kissat_eliminate_clause (solver, d, not_lit);
	}
    }
  if (optimize && neg_watches->size)
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

  LOG ("next variable elimination candidate %u", idx);

  LOG ("marking variable %u as not removed", idx);
  flags->eliminate = false;

  return true;
}

static bool
eliminate_variable (kissat * solver, unsigned idx)
{
  assert (!solver->inconsistent);
  if (!can_eliminate_variable (solver, idx))
    return false;
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
      INC (gates);
#ifndef NMETRICS
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

  unsigned eliminated = 0;
  const unsigned active = solver->active;

  SET_EFFICIENCY_BOUND (resolution_limit, eliminate,
			resolutions, search_ticks,
			2 * CLAUSES + kissat_nlogn (active));

  bool complete = false;
  int round = 0;

  const bool forward = GET_OPTION (forward);

  for (;;)
    {
      if (forward)
	{
	  unsigned propagated = solver->propagated;
	  kissat_forward_subsume_during_elimination (solver);
	  if (solver->inconsistent)
	    break;
	  kissat_flush_large_connected (solver);
	  kissat_connect_irredundant_large_clauses (solver);
	  solver->propagated = propagated;
	  kissat_flush_units_while_connected (solver);
	  if (solver->inconsistent)
	    break;
	}
      else
	kissat_connect_irredundant_large_clauses (solver);
      if (!schedule_variables (solver))
	{
	  kissat_release_heap (solver, &solver->schedule);
	  complete = true;
	  break;
	}
      round++;
#ifndef QUIET
      LOG ("entering variable elimination round %d", round);
      const unsigned before = eliminated;
#endif
      while (!kissat_empty_heap (&solver->schedule))
	{
	  if (solver->statistics.resolutions > resolution_limit)
	    break;
	  if (TERMINATED (5))
	    break;
	  unsigned idx = kissat_max_heap (&solver->schedule);
	  kissat_pop_heap (solver, &solver->schedule, idx);
	  if (eliminate_variable (solver, idx))
	    eliminated++;
	  if (solver->inconsistent)
	    break;
	}
      if (!solver->inconsistent)
	{
	  kissat_flush_large_connected (solver);
	  kissat_dense_collect (solver);
	}
      REPORT (before == eliminated, 'e');
      if (solver->inconsistent)
	break;
      kissat_release_heap (solver, &solver->schedule);
      if (round == GET_OPTION (eliminaterounds))
	break;
      if (solver->statistics.resolutions > resolution_limit)
	break;
      if (TERMINATED (6))
	break;
    }
  kissat_phase (solver, "eliminate", GET (eliminations),
		"eliminated %u variables %.0f%% out of %u in %zu rounds",
		eliminated, kissat_percent (eliminated, active), active,
		round);
  if (!solver->inconsistent)
    set_next_elimination_bound (solver, complete);
}

static void
setup_elim_bounds (kissat * solver)
{
  solver->bounds.eliminate.clause_size =
    SCALE_LIMIT (eliminations, eliminateclslim);

  solver->bounds.eliminate.occurrences =
    SCALE_LIMIT (eliminations, eliminateocclim);

  kissat_phase (solver, "eliminate", GET (eliminations),
		"occurrence limit %u and clause limit %u",
		solver->bounds.eliminate.occurrences,
		solver->bounds.eliminate.clause_size);

  solver->bounds.subsume.clause_size =
    SCALE_LIMIT (eliminations, subsumeclslim);

  solver->bounds.subsume.occurrences =
    SCALE_LIMIT (eliminations, subsumeocclim);

  kissat_phase (solver, "subsume", GET (eliminations),
		"occurrence limit %u clause limit %u",
		solver->bounds.subsume.occurrences,
		solver->bounds.subsume.clause_size);

  solver->bounds.xor.clause_size =
    log (MAX (solver->bounds.eliminate.occurrences, 1)) / log (2.0) + 0.5;
  if (solver->bounds.xor.clause_size > (unsigned) GET_OPTION (xorsclslim))
    solver->bounds.xor.clause_size = (unsigned) GET_OPTION (xorsclslim);
  assert (solver->bounds.xor.clause_size < 32);

  LOG ("maximum XOR base clause size %u", solver->bounds.xor.clause_size);
}

static void
eliminate (kissat * solver)
{
  RETURN_IF_DELAYED (eliminate);
  kissat_backtrack_propagate_and_flush_trail (solver);
  assert (!solver->inconsistent);
  STOP_SEARCH_AND_START_SIMPLIFIER (eliminate);
  kissat_phase (solver, "eliminate", GET (eliminations),
		"elimination limit of %" PRIu64 " conflicts hit",
		solver->limits.eliminate.conflicts);
  const changes before = kissat_changes (solver);
  setup_elim_bounds (solver);
  litwatches saved;
  INIT_STACK (saved);
  kissat_enter_dense_mode (solver, 0, &saved);
  eliminate_variables (solver);
  kissat_resume_sparse_mode (solver, true, 0, &saved);
  RELEASE_STACK (saved);
  kissat_check_statistics (solver);
  const changes after = kissat_changes (solver);
  const bool changed = kissat_changed (before, after);
  UPDATE_DELAY (changed, eliminate);
  STOP_SIMPLIFIER_AND_RESUME_SEARCH (eliminate);
}

int
kissat_eliminate (kissat * solver)
{
  assert (!solver->inconsistent);
  INC (eliminations);
  eliminate (solver);
  UPDATE_CONFLICT_LIMIT (eliminate, eliminations, NLOGNLOGN, true);
  solver->waiting.eliminate.reduce = solver->statistics.reductions + 1;
  return solver->inconsistent ? 20 : 0;
}
