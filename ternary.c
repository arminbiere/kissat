#include "allocate.h"
#include "dense.h"
#include "eliminate.h"
#include "inline.h"
#include "print.h"
#include "ternary.h"
#include "report.h"
#include "sort.h"
#include "terminate.h"

static void
connect_ternary_clauses (kissat * solver, value * ternary)
{
  assert (!solver->level);
  assert (!solver->watching);
  size_t connected = 0;
  value *values = solver->values;
  for (all_clauses (c))
    {
      if (c->garbage)
	continue;
      if (c->size != 3)
	continue;
      const unsigned *lits = c->lits;
      if (values[lits[0]])
	continue;
      if (values[lits[1]])
	continue;
      if (values[lits[2]])
	continue;
      ternary[lits[0]] = ternary[lits[1]] = ternary[lits[2]] = 1;
      kissat_connect_clause (solver, c);
      connected++;
    }
}

static unsigned
schedule_ternary (kissat * solver, value * ternary)
{
  heap *schedule = &solver->schedule;
  flags *flags = solver->flags;

  assert (!schedule->size);
  unsigned scheduled = 0;

  for (all_variables (idx))
    {
      struct flags *f = flags + idx;
      if (!f->active)
	continue;
      const unsigned lit = LIT (idx);
      if (!ternary[lit])
	continue;
      const unsigned not_lit = NOT (lit);
      if (!ternary[not_lit])
	continue;
      if (!schedule->size)
	kissat_resize_heap (solver, schedule, solver->vars);
      kissat_update_variable_score (solver, schedule, idx);
      kissat_push_heap (solver, schedule, idx);
      scheduled++;
    }

  if (!scheduled)
    return 0;

  kissat_phase (solver, "ternary", GET (hyper_ternary_phases),
		"scheduled %u variables %.0f%%",
		scheduled, kissat_percent (scheduled, solver->active));

  return scheduled;
}

typedef struct tag tag;

struct tag
{
  unsigned first:30;
  bool redundant:1;
  bool binary:1;
};

// *INDENT-OFF*
typedef STACK (tag) tags;
// *INDENT-ON*

static void
ternary_resolution (kissat * solver, tags * tags, references * garbage,
		    unsigned lit, clause * c, clause * d)
{
  assert (c->size == 3);
  assert (d->size == 3);

  LOGCLS (c, "ternary 1st antecedent");
  LOGCLS (d, "ternary 2nd antecedent");

  INC (hyper_ternary_resolutions);

  unsigned lits[4];
  unsigned size = 0;
  for (unsigned i = 0; i < 3; i++)
    {
      const unsigned other = c->lits[i];
      if (other == lit)
	continue;
      lits[size++] = other;
    }
  assert (size == 2);

  const unsigned not_lit = NOT (lit);
  bool tautological = false;
  for (unsigned i = 0; i < 3; i++)
    {
      const unsigned other = d->lits[i];
      if (other == not_lit)
	continue;
      const unsigned not_other = NOT (other);
      bool found = false;
      for (unsigned j = 0; j < size; j++)
	{
	  const unsigned prev = lits[j];
	  if (other == prev)
	    {
	      found = true;
	      break;
	    }
	  if (not_other == prev)
	    {
	      tautological = true;
#ifndef LOGGING
	      break;
#endif
	    }
	}
#ifndef LOGGING
      if (tautological)
	break;
#endif
      if (found)
	continue;
      assert (size < 4);
      lits[size++] = other;
    }

  if (tautological)
    {
      LOGLITS (size, lits, "tautological resolvent");
      return;
    }

  if (size == 4)
    {
      LOGLITS (4, lits, "size 3 exceeding resolvent");
      return;
    }

  assert (size == 2 || size == 3);
  LOGLITS (size, lits, "ternary resolvent");

  for (unsigned i = size - 1; i; i--)
    PUSH_STACK (solver->delayed, lits[i]);

  bool redundant;
  if (size == 3)
    redundant = true;
  else
    {
      redundant = c->redundant && d->redundant;
      const reference c_ref = kissat_reference_clause (solver, c);
      const reference d_ref = kissat_reference_clause (solver, d);
      PUSH_STACK (*garbage, c_ref);
      PUSH_STACK (*garbage, d_ref);
      assert (!c->garbage), assert (!d->garbage);
      c->garbage = d->garbage = true;
    }

  tag tag;
  tag.first = lits[0];
  tag.redundant = redundant;
  tag.binary = (size == 2);

  PUSH_STACK (*tags, tag);
}

static bool
find_binary (kissat * solver, bool irredundant,
	     unsigned first, unsigned second)
{
  if (WATCHES (first).size > WATCHES (second).size)
    SWAP (unsigned, first, second);
  watches *watches = &WATCHES (first);
  const unsigned steps = kissat_cache_lines (watches->size, sizeof (watch));
  ADD (hyper_ternary_steps, steps + 1);
  for (all_binary_large_watches (watch, *watches))
    {
      if (!watch.type.binary)
	continue;
      if (irredundant && watch.binary.redundant)
	continue;
      const unsigned other = watch.binary.lit;
      if (other == second)
	return true;
    }
  return false;
}

static bool
find_ternary (kissat * solver, word * arena,
	      unsigned first, unsigned second, unsigned third)
{
  if (WATCHES (second).size > WATCHES (third).size)
    SWAP (unsigned, second, third);
  if (WATCHES (first).size > WATCHES (second).size)
    SWAP (unsigned, first, second);
  watches *watches = &WATCHES (first);
  const unsigned steps = kissat_cache_lines (watches->size, sizeof (watch));
  ADD (hyper_ternary_steps, steps + 1);
  for (all_binary_large_watches (watch, *watches))
    {
      if (watch.type.binary)
	{
	  const unsigned other = watch.binary.lit;
	  if (other == second)
	    return true;
	  if (other == third)
	    return true;
	}
      else
	{
	  const reference ref = watch.large.ref;
	  clause *d = (clause *) (arena + ref);
	  assert (kissat_clause_in_arena (solver, d));
	  assert (d->size == 3);
	  INC (hyper_ternary_steps);

	  const unsigned a = d->lits[0];
	  const unsigned b = d->lits[1];
	  const unsigned c = d->lits[2];
	  if (a == first)
	    {
	      if (b == second && c == third)
		return true;
	      if (c == second && b == third)
		return true;
	    }
	  else if (b == first)
	    {
	      if (a == second && c == third)
		return true;
	      if (c == second && a == third)
		return true;
	    }
	  else if (c == first)
	    {
	      if (a == second && b == third)
		return true;
	      if (b == second && a == third)
		return true;
	    }
	}
    }
  return find_binary (solver, false, second, third);
}

static void
update_ternary_schedule_literal (kissat * solver, heap * schedule,
				 bool reschedule, unsigned lit)
{
  const unsigned idx = IDX (lit);
  kissat_update_variable_score (solver, schedule, idx);
  if (reschedule && !kissat_heap_contains (schedule, idx))
    kissat_push_heap (solver, schedule, idx);
}

static void
update_ternary_schedule_stack (kissat * solver,
			       bool reschedule, unsigneds * stack)
{
  heap *schedule = &solver->schedule;
  for (all_stack (unsigned, lit, *stack))
      update_ternary_schedule_literal (solver, schedule, reschedule, lit);
}

static word *
add_ternary_resolvents (kissat * solver, tags * tags, uint64_t * resolved_ptr)
{
  word *arena = BEGIN_STACK (solver->arena);
  while (!EMPTY_STACK (*tags))
    {
      tag tag = POP_STACK (*tags);
      const bool binary = tag.binary;
      const bool redundant = tag.redundant;
      const unsigned first = tag.first;
      const unsigned second = POP_STACK (solver->delayed);
      const unsigned third =
	binary ? INVALID_LIT : POP_STACK (solver->delayed);
      if (binary)
	{
	  if (find_binary (solver, !redundant, first, second))
	    continue;
	}
      else
	{
	  assert (third != INVALID_LIT);
	  if (find_ternary (solver, arena, first, second, third))
	    continue;
	}
      assert (EMPTY_STACK (solver->clause.lits));
      PUSH_STACK (solver->clause.lits, first);
      PUSH_STACK (solver->clause.lits, second);
      if (!binary)
	PUSH_STACK (solver->clause.lits, third);
      if (!redundant)
	{
	  assert (binary);
	  (void) kissat_new_irredundant_clause (solver);
	}
      else if (binary)
	{
	  (void) kissat_new_binary_clause (solver, true, first, second);
	  assert (arena == BEGIN_STACK (solver->arena));
	}
      else
	{
	  reference ref = kissat_new_redundant_clause (solver, 2);
	  clause *c = kissat_dereference_clause (solver, ref);
	  assert (c->redundant);
	  assert (c->size == 3);
	  INC (hyper_ternaries);
	  assert (!c->hyper);
	  c->hyper = true;
	  LOGCLS (c, "actually");
	}
      arena = BEGIN_STACK (solver->arena);

      if (binary)
	INC (hyper_ternary2_resolved);
      else
	INC (hyper_ternary3_resolved);
      INC (hyper_ternary_resolved);
      *resolved_ptr += 1;

      update_ternary_schedule_stack (solver, !binary, &solver->clause.lits);
      CLEAR_STACK (solver->clause.lits);
    }
  assert (EMPTY_STACK (solver->delayed));

  return arena;
}

static void
remove_ternary_subsumed_clauses (kissat * solver, references * garbage)
{
  size_t marked = 0;
  for (all_stack (reference, ref, *garbage))
    {
      clause *c = kissat_dereference_clause (solver, ref);
      assert (c->garbage);
      c->garbage = false;
      kissat_mark_clause_as_garbage (solver, c);
      marked++;
    }
  LOG ("marked %zu clauses as garbage", marked);
  CLEAR_STACK (*garbage);
  (void) marked;
}

static word *
resolve_ternary_clauses (kissat * solver, word * arena, uint64_t steps_limit,
			 tags * tags, references * garbage, unsigned lit,
			 uint64_t * resolved_ptr)
{
  assert (!solver->level);
  LOG ("resolving ternary clauses on literal %s", LOGLIT (lit));
  const unsigned not_lit = NOT (lit);

  watches *pos_watches = &WATCHES (lit);
  watches *neg_watches = &WATCHES (not_lit);

  assert (EMPTY_STACK (*garbage));
  assert (EMPTY_STACK (solver->delayed));

  for (all_binary_large_watches (pos_watch, *pos_watches))
    {
      if (TERMINATED (13))
	break;
      if (solver->statistics.hyper_ternary_steps > steps_limit)
	break;
      if (pos_watch.type.binary)
	continue;
      const reference pos_ref = pos_watch.large.ref;
      clause *c = (clause *) (arena + pos_ref);
      assert (kissat_clause_in_arena (solver, c));
      assert (c->size == 3);
      INC (hyper_ternary_steps);
      if (c->garbage)
	continue;
      for (all_binary_large_watches (neg_watch, *neg_watches))
	{
	  if (neg_watch.type.binary)
	    continue;
	  const reference neg_ref = neg_watch.large.ref;
	  clause *d = (clause *) (arena + neg_ref);
	  assert (kissat_clause_in_arena (solver, d));
	  assert (d->size == 3);
	  assert (c != d);
	  INC (hyper_ternary_steps);
	  if (d->garbage)
	    continue;
	  ternary_resolution (solver, tags, garbage, lit, c, d);
	  if (c->garbage)
	    break;
	}
    }

  word *res = add_ternary_resolvents (solver, tags, resolved_ptr);
  remove_ternary_subsumed_clauses (solver, garbage);

  return res;
}

static bool
really_ternary (kissat * solver)
{
  if (!GET_OPTION (really))
    return true;

  const uint64_t limit = 2 * CLAUSES;
  statistics *statistics = &solver->statistics;

  if (limit >= statistics->search_ticks + GET_OPTION (ternarymineff))
    return false;

  for (all_clauses (c))
    if (!c->garbage && c->size == 3)
      return true;

  return false;
}

static uint64_t
ternary_round (kissat * solver,
	       const uint64_t resolved_limit, const uint64_t steps_limit)
{
  tags tags;
  INIT_STACK (tags);

  references garbage;
  INIT_STACK (garbage);

  word *arena = BEGIN_STACK (solver->arena);
  heap *schedule = &solver->schedule;

#ifndef QUIET
  size_t scheduled = kissat_size_heap (schedule);
#endif
  uint64_t resolved = 0;

  while (!kissat_empty_heap (schedule))
    {
      if (TERMINATED (14))
	break;

      if (solver->statistics.hyper_ternary_steps > steps_limit)
	{
	  kissat_phase (solver, "ternary", GET (hyper_ternary_phases),
			"steps limit %" PRIu64 " reached", steps_limit);
	  break;
	}

      if (resolved > resolved_limit)
	{
	  kissat_phase (solver, "ternary", GET (hyper_ternary_phases),
			"resolved limit %" PRIu64 " reached", resolved_limit);
	  break;
	}

      const unsigned idx = kissat_max_heap (schedule);
      kissat_pop_heap (solver, schedule, idx);
      assert (ACTIVE (idx));

      const unsigned lit = LIT (idx);
      arena = resolve_ternary_clauses (solver, arena, steps_limit,
				       &tags, &garbage, lit, &resolved);
    }

#ifndef QUIET
  size_t remain = kissat_size_heap (schedule);
  if (remain)
    kissat_phase (solver, "ternary", GET (hyper_ternary_phases),
		  "remaining %u variables %.0f%% (incomplete ternary round)",
		  remain, kissat_percent (remain, scheduled));
  else
    kissat_phase (solver, "ternary", GET (hyper_ternary_phases),
		  "all %u variables resolved (complete ternary round)",
		  scheduled);
#endif

  RELEASE_STACK (tags);
  RELEASE_STACK (garbage);

  return resolved;
}

void
kissat_ternary (kissat * solver)
{
  if (solver->inconsistent)
    return;

  assert (!solver->level);
  assert (solver->watching);
  assert (solver->probing);

  if (TERMINATED (15))
    return;
  if (!GET_OPTION (ternary))
    return;

  if (!really_ternary (solver))
    return;

  RETURN_IF_DELAYED (ternary);

  START (ternary);
  INC (hyper_ternary_phases);

#ifndef NMETRICS
  uint64_t resolved2 = solver->statistics.hyper_ternary2_resolved;
  uint64_t resolved3 = solver->statistics.hyper_ternary3_resolved;
#endif

  kissat_enter_dense_mode (solver, 0, 0);

  value *ternary = kissat_calloc (solver, LITS, sizeof *ternary);
  connect_ternary_clauses (solver, ternary);

  const double resolved_limit_fraction = GET_OPTION (ternarymaxadd) * 0.01;
  const uint64_t resolved_limit = CLAUSES * resolved_limit_fraction;

  const unsigned scheduled = schedule_ternary (solver, ternary);
  uint64_t resolved = 0;

  if (scheduled)
    {
      SET_EFFICIENCY_BOUND (steps_limit, ternary,
			    hyper_ternary_steps, search_ticks,
			    2 * CLAUSES + kissat_nlogn (scheduled));

      resolved = ternary_round (solver, resolved_limit, steps_limit);
      kissat_release_heap (solver, &solver->schedule);
    }

  kissat_dealloc (solver, ternary, LITS, sizeof *ternary);
  kissat_resume_sparse_mode (solver, false, 0, 0);

#ifndef NMETRICS
  resolved2 = solver->statistics.hyper_ternary2_resolved - resolved2;
  resolved3 = solver->statistics.hyper_ternary3_resolved - resolved3;
  kissat_phase (solver, "ternary", GET (hyper_ternary_phases),
		"resolved %" PRIu64 " clauses (%" PRIu64
		" ternary %.0f%% and %" PRIu64 " binary %.0f%%)",
		resolved,
		resolved3, kissat_percent (resolved3, resolved),
		resolved2, kissat_percent (resolved2, resolved));
#else
  kissat_phase (solver, "ternary", GET (hyper_ternary_phases),
		"resolved %" PRIu64 " clauses", resolved);
#endif
  UPDATE_DELAY (resolved, ternary);
  REPORT (!resolved, '3');
  kissat_check_statistics (solver);
  STOP (ternary);
}
