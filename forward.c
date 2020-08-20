#include "allocate.h"
#include "eliminate.h"
#include "forward.h"
#include "inline.h"
#include "print.h"
#include "rank.h"
#include "sort.h"
#include "report.h"
#include "terminate.h"

#include <inttypes.h>

static size_t
remove_duplicated_binaries_with_literal (kissat * solver, unsigned lit)
{
  watches *watches = &WATCHES (lit);
  value *marks = solver->marks;
  flags *flags = solver->flags;

  watch *begin = BEGIN_WATCHES (*watches), *q = begin;
  const watch *end = END_WATCHES (*watches), *p = q;

  while (p != end)
    {
      const watch watch = *q++ = *p++;
      assert (watch.type.binary);
      const unsigned other = watch.binary.lit;
      struct flags *f = flags + IDX (other);
      if (!f->active)
	continue;
      if (!f->subsume)
	continue;
      const value marked = marks[other];
      if (marked)
	{
	  q--;
	  if (lit < other)
	    {
	      kissat_delete_binary (solver, false, false, lit, other);
	      INC (duplicated);
	    }
	}
      else
	{
	  const unsigned not_other = NOT (other);
	  if (marks[not_other])
	    {
	      LOGBINARY (lit, other,
			 "duplicate hyper unary resolution on %s "
			 "first antecedent", LOGLIT (other));
	      LOGBINARY (lit, not_other,
			 "duplicate hyper unary resolution on %s "
			 "second antecedent", LOGLIT (not_other));
	      PUSH_STACK (solver->delayed, lit);
	    }
	  marks[other] = 1;
	}
    }

  for (const watch * r = begin; r != q; r++)
    marks[r->binary.lit] = 0;

  if (q == end)
    return 0;

  size_t removed = end - q;
  SET_END_OF_WATCHES (*watches, q);
  LOG ("removed %zu watches with literal %s", removed, LOGLIT (lit));

  return removed;
}

static void
remove_all_duplicated_binary_clauses (kissat * solver)
{
  LOG ("removing all duplicated irredundant binary clauses");
  size_t removed = 0;
  assert (EMPTY_STACK (solver->delayed));

  const flags *all_flags = solver->flags;

  for (all_variables (idx))
    {
      const flags *flags = all_flags + idx;
      if (!flags->active)
	continue;
      if (!flags->subsume)
	continue;
      const unsigned int lit = LIT (idx);
      const unsigned int not_lit = NOT (lit);
      removed += remove_duplicated_binaries_with_literal (solver, lit);
      removed += remove_duplicated_binaries_with_literal (solver, not_lit);
    }
  assert (!(removed & 1));

  size_t units = SIZE_STACK (solver->delayed);
  if (units)
    {
      LOG ("found %zu hyper unary resolved units", units);
      const value *values = solver->values;
      for (all_stack (unsigned, unit, solver->delayed))
	{

	  const value value = values[unit];
	  if (value > 0)
	    {
	      LOG ("skipping satisfied resolved unit %s", LOGLIT (unit));
	      continue;
	    }
	  if (value < 0)
	    {
	      LOG ("found falsified resolved unit %s", LOGLIT (unit));
	      CHECK_AND_ADD_EMPTY ();
	      ADD_EMPTY_TO_PROOF ();
	      solver->inconsistent = true;
	      break;
	    }
	  LOG ("new resolved unit clause %s", LOGLIT (unit));
	  kissat_assign_unit (solver, unit);
	  CHECK_AND_ADD_UNIT (unit);
	  ADD_UNIT_TO_PROOF (unit);
	}
      CLEAR_STACK (solver->delayed);
      if (!solver->inconsistent)
	kissat_flush_units_while_connected (solver);
    }

  REPORT (!removed && !units, '2');
}

static void
find_forward_subsumption_candidates (kissat * solver, references * candidates)
{
  const unsigned clslim = solver->bounds.subsume.clause_size;
  size_t left_over_from_last_subsumption_round = 0;

  const value *values = solver->values;
  const flags *flags = solver->flags;

  clause *last_irredundant = kissat_last_irredundant_clause (solver);

  for (all_clauses (c))
    {
      if (last_irredundant && c > last_irredundant)
	break;
      if (c->redundant)
	continue;
      if (c->garbage)
	continue;
      if (c->size > clslim)
	continue;
      assert (c->size > 2);
      unsigned subsume = 0;
      for (all_literals_in_clause (lit, c))
	{
	  const unsigned idx = IDX (lit);
	  const struct flags *f = flags + idx;
	  if (f->subsume)
	    subsume++;
	  if (values[lit] > 0)
	    {
	      LOGCLS (c, "satisfied by %s", LOGLIT (lit));
	      kissat_mark_clause_as_garbage (solver, c);
	      assert (c->garbage);
	      break;
	    }
	}
      if (c->garbage)
	continue;
      if (subsume < 2)
	continue;
      if (c->subsume)
	left_over_from_last_subsumption_round++;
      const unsigned ref = kissat_reference_clause (solver, c);
      PUSH_STACK (*candidates, ref);
    }

  if (left_over_from_last_subsumption_round)
    return;

  for (all_stack (reference, ref, *candidates))
    kissat_dereference_clause (solver, ref)->subsume = true;
}

static inline unsigned
get_size_of_reference (kissat * solver, const word * arena, reference ref)
{
  assert (ref < SIZE_STACK (solver->arena));
  const clause *c = (clause *) (arena + ref);
  (void) solver;
  return c->size;
}

#define GET_SIZE_OF_REFERENCE(REF) \
  get_size_of_reference (solver, arena, (REF))

#define RADIX_SORT_FORWARD_LENGTH 16

static void
sort_forward_subsumption_candidates (kissat * solver, references * candidates)
{
  reference *references = BEGIN_STACK (*candidates);
  size_t size = SIZE_STACK (*candidates);
  const word *arena = BEGIN_STACK (solver->arena);
  RADIX (RADIX_SORT_FORWARD_LENGTH, reference,
	 unsigned, size, references, GET_SIZE_OF_REFERENCE);
}

static inline bool
forward_literal (kissat * solver, unsigned lit, bool binaries,
		 unsigned *remove)
{
  watches *watches = &WATCHES (lit);
  const unsigned occs = watches->size;

  if (!occs)
    return false;

  if (occs > solver->bounds.subsume.occurrences)
    return false;

  watch *begin = BEGIN_WATCHES (*watches), *q = begin;
  const watch *end = END_WATCHES (*watches), *p = q;

  const value *values = solver->values;
  const value *marks = solver->marks;
  const word *arena = BEGIN_STACK (solver->arena);

  bool subsume = false;

  while (p != end)
    {
      const watch watch = *q++ = *p++;

      if (watch.type.binary)
	{
	  if (!binaries)
	    continue;

	  const unsigned other = watch.binary.lit;
	  if (marks[other])
	    {
	      LOGBINARY (lit, other, "forward subsuming");
	      subsume = true;
	      break;
	    }
	  else
	    {
	      const unsigned not_other = NOT (other);
	      if (marks[not_other])
		{
		  LOGBINARY (lit, other,
			     "forward %s strengthener", LOGLIT (other));
		  assert (!subsume);
		  *remove = not_other;
		  break;
		}
	    }
	}
      else
	{
	  const reference ref = watch.large.ref;
	  assert (ref < SIZE_STACK (solver->arena));
	  clause *d = (clause *) (arena + ref);
	  if (d->garbage)
	    {
	      q--;
	      continue;
	    }

	  INC (subsumption_checks);

	  subsume = true;

	  unsigned candidate = INVALID_LIT;

	  for (all_literals_in_clause (other, d))
	    {
	      if (marks[other])
		continue;
	      const value value = values[other];
	      if (value < 0)
		continue;
	      if (value > 0)
		{
		  LOGCLS (d, "satisfied by %s", LOGLIT (other));
		  kissat_mark_clause_as_garbage (solver, d);
		  assert (d->garbage);
		  candidate = INVALID_LIT;
		  subsume = false;
		  break;
		}
	      if (!subsume)
		{
		  assert (candidate != INVALID_LIT);
		  candidate = INVALID_LIT;
		  break;
		}
	      subsume = false;
	      const unsigned not_other = NOT (other);
	      if (!marks[not_other])
		{
		  assert (candidate == INVALID_LIT);
		  break;
		}
	      candidate = not_other;
	    }

	  if (d->garbage)
	    {
	      assert (!subsume);
	      q--;
	      break;
	    }

	  if (subsume)
	    {
	      LOGCLS (d, "forward subsuming");
	      assert (subsume);
	      break;
	    }

	  if (candidate != INVALID_LIT)
	    {
	      LOGCLS (d, "forward %s strengthener", LOGLIT (candidate));
	      *remove = candidate;
	    }
	}
    }

  while (p != end)
    *q++ = *p++;

  SET_END_OF_WATCHES (*watches, q);

  return subsume;
}

static inline bool
forward_marked_clause (kissat * solver, clause * c, unsigned *remove)
{
  const flags *flags = solver->flags;

  for (all_literals_in_clause (lit, c))
    {
      const unsigned idx = IDX (lit);
      if (!flags[idx].active)
	continue;

      assert (!VALUE (lit));

      if (forward_literal (solver, lit, true, remove))
	return true;
      if (forward_literal (solver, NOT (lit), false, remove))
	return true;
    }
  return false;
}

static bool
forward_subsumed_clause (kissat * solver, clause * c, bool * removed)
{
  assert (!c->garbage);
  LOGCLS2 (c, "trying to forward subsume");

  value *marks = solver->marks;
  const value *values = solver->values;
  unsigned non_false = 0, unit = INVALID_LIT;

  for (all_literals_in_clause (lit, c))
    {
      const value value = values[lit];
      if (value < 0)
	continue;
      if (value > 0)
	{
	  LOGCLS (c, "satisfied by %s", LOGLIT (lit));
	  kissat_mark_clause_as_garbage (solver, c);
	  assert (c->garbage);
	  break;
	}
      marks[lit] = 1;
      if (non_false++)
	unit ^= lit;
      else
	unit = lit;
    }

  if (c->garbage || non_false <= 1)
    for (all_literals_in_clause (lit, c))
      marks[lit] = 0;

  if (c->garbage)
    return false;

  if (!non_false)
    {
      LOGCLS (c, "found falsified clause");
      CHECK_AND_ADD_EMPTY ();
      ADD_EMPTY_TO_PROOF ();
      solver->inconsistent = true;
      return false;
    }

  if (non_false == 1)
    {
      assert (VALID_INTERNAL_LITERAL (unit));
      LOG ("new remaining non-false literal unit clause %s", LOGLIT (unit));
      kissat_assign_unit (solver, unit);
      CHECK_AND_ADD_UNIT (unit);
      ADD_UNIT_TO_PROOF (unit);
      kissat_mark_clause_as_garbage (solver, c);
      kissat_flush_units_while_connected (solver);
      return false;
    }

  unsigned remove = INVALID_LIT;
  const bool subsume = forward_marked_clause (solver, c, &remove);

  for (all_literals_in_clause (lit, c))
    marks[lit] = 0;

  if (subsume)
    {
      LOGCLS (c, "forward subsumed");
      kissat_mark_clause_as_garbage (solver, c);
      INC (subsumed);
      INC (forward_subsumed);
    }
  else if (remove != INVALID_LIT)
    {
      INC (strengthened);
      INC (forward_strengthened);
      LOGCLS (c, "forward strengthening by removing %s in", LOGLIT (remove));
      if (non_false == 2)
	{
	  unit ^= remove;
	  assert (VALID_INTERNAL_LITERAL (unit));
	  LOG ("forward strengthened unit clause %s", LOGLIT (unit));
	  kissat_assign_unit (solver, unit);
	  CHECK_AND_ADD_UNIT (unit);
	  ADD_UNIT_TO_PROOF (unit);
	  kissat_mark_clause_as_garbage (solver, c);
	  *removed = true;
	  kissat_flush_units_while_connected (solver);
	  LOGCLS (c, "%s satisfied", LOGLIT (unit));
	}
      else
	{
	  SHRINK_CLAUSE_IN_PROOF (c, remove, INVALID_LIT);
	  CHECK_SHRINK_CLAUSE (c, remove, INVALID_LIT);
	  kissat_mark_removed_literal (solver, remove);
	  if (non_false > 3)
	    {
	      unsigned *lits = c->lits;
	      unsigned new_size = 0;
	      for (unsigned i = 0; i < c->size; i++)
		{
		  const unsigned lit = lits[i];
		  if (remove == lit)
		    continue;
		  const value value = values[lit];
		  if (value < 0)
		    continue;
		  assert (!value);
		  lits[new_size++] = lit;
		  kissat_mark_added_literal (solver, lit);
		}
	      assert (new_size == non_false - 1);
	      assert (new_size > 2);
	      if (!c->shrunken)
		{
		  c->shrunken = true;
		  lits[c->size - 1] = INVALID_LIT;
		}
	      c->size = new_size;
	      c->searched = 2;
	      LOGCLS (c, "forward strengthened");
	    }
	  else
	    {
	      assert (non_false == 3);
	      LOGCLS (c, "garbage");
	      assert (!c->garbage);
	      assert (!c->hyper);
	      const size_t bytes = kissat_actual_bytes_of_clause (c);
	      ADD (arena_garbage, bytes);
	      c->garbage = true;
	      unsigned first = INVALID_LIT, second = INVALID_LIT;
	      for (all_literals_in_clause (lit, c))
		{
		  if (lit == remove)
		    continue;
		  const value value = values[lit];
		  if (value < 0)
		    continue;
		  assert (!value);
		  if (first == INVALID_LIT)
		    first = lit;
		  else
		    {
		      assert (second == INVALID_LIT);
		      second = lit;
		    }
		  kissat_mark_added_literal (solver, lit);
		}
	      assert (first != INVALID_LIT);
	      assert (second != INVALID_LIT);
	      LOGBINARY (first, second, "forward strengthened");
	      kissat_watch_other (solver, false, false, first, second);
	      kissat_watch_other (solver, false, false, second, first);
	      *removed = true;
	    }
	}
    }

  return subsume;
}

static void
connect_subsuming (kissat * solver, unsigned occlim, clause * c)
{
  assert (!c->garbage);

  unsigned min_lit = INVALID_LIT;
  unsigned min_occs = UINT_MAX;

  const flags *all_flags = solver->flags;

  bool subsume = true;

  for (all_literals_in_clause (lit, c))
    {
      const unsigned idx = IDX (lit);
      const flags *flags = all_flags + idx;
      if (!flags->active)
	continue;
      if (!flags->subsume)
	{
	  subsume = false;
	  break;
	}
      watches *watches = &WATCHES (lit);
      const unsigned occs = watches->size;
      if (min_lit != INVALID_LIT && occs > min_occs)
	continue;
      min_lit = lit;
      min_occs = occs;
    }
  if (!subsume)
    return;

  if (min_occs > occlim)
    return;
  LOG ("connecting %s with %u occurrences", LOGLIT (min_lit), min_occs);
  const reference ref = kissat_reference_clause (solver, c);
  kissat_connect_literal (solver, min_lit, ref);
}

static void
forward_subsume_all_clauses (kissat * solver)
{
  references candidates;
  INIT_STACK (candidates);

  find_forward_subsumption_candidates (solver, &candidates);
  size_t scheduled = SIZE_STACK (candidates);

  kissat_phase (solver, "forward", GET (forward_subsumptions),
		"scheduled %zu irredundant clauses %.0f%%", scheduled,
		kissat_percent (scheduled,
				solver->statistics.clauses_irredundant));

  sort_forward_subsumption_candidates (solver, &candidates);

  const reference *end_of_candidates = END_STACK (candidates);
  reference *p = BEGIN_STACK (candidates);

  size_t subsumed = 0;
  size_t strengthened = 0;
#ifndef QUIET
  size_t checked = 0;
#endif
  const unsigned occlim = solver->bounds.subsume.occurrences;

  {
    SET_EFFICIENCY_BOUND (limit, subsume, subsumption_checks,
			  search_ticks, kissat_nlogn (scheduled));

    word *arena = BEGIN_STACK (solver->arena);

    while (p != end_of_candidates)
      {
	if (solver->statistics.subsumption_checks > limit)
	  break;
	if (TERMINATED (9))
	  break;
	reference ref = *p++;
	clause *c = (clause *) (arena + ref);
	assert (kissat_clause_in_arena (solver, c));
	assert (!c->garbage);
	if (!c->subsume)
	  continue;
	c->subsume = false;
#ifndef QUIET
	checked++;
#endif
	bool removed = false;
	if (forward_subsumed_clause (solver, c, &removed))
	  subsumed++;
	else if (removed)
	  strengthened++;
	if (solver->inconsistent)
	  break;
	if (!c->garbage)
	  connect_subsuming (solver, occlim, c);
      }
  }
#ifndef QUIET
  if (subsumed)
    kissat_phase (solver, "forward", GET (forward_subsumptions),
		  "subsumed %zu clauses %.2f%% of %zu checked %.0f%%",
		  subsumed, kissat_percent (subsumed, checked),
		  checked, kissat_percent (checked, scheduled));
  if (strengthened)
    kissat_phase (solver, "forward", GET (forward_subsumptions),
		  "strengthened %zu clauses %.2f%% of %zu checked %.0f%%",
		  strengthened, kissat_percent (strengthened, checked),
		  checked, kissat_percent (checked, scheduled));
  if (!subsumed && !strengthened)
    kissat_phase (solver, "forward", GET (forward_subsumptions),
		  "no clause subsumed nor strengthened "
		  "out of %zu checked %.0f%%",
		  checked, kissat_percent (checked, scheduled));
#endif
  struct flags *flags = solver->flags;
  struct flags *saved = kissat_calloc (solver, solver->vars, sizeof *saved);
  memcpy (saved, flags, solver->vars * sizeof *saved);

  for (all_variables (idx))
    flags[idx].subsume = false;

  {
#ifndef QUIET
    size_t remain = 0;
#endif
    word *arena = BEGIN_STACK (solver->arena);

    while (p != end_of_candidates)
      {
	const reference ref = *p++;
	clause *c = (clause *) (arena + ref);
	assert (kissat_clause_in_arena (solver, c));
	assert (!c->garbage);
#ifndef QUIET
	if (c->subsume)
	  remain++;
#endif
	for (all_literals_in_clause (lit, c))
	  {
	    const unsigned idx = IDX (lit);
	    flags[idx].subsume = saved[idx].subsume;
	  }
      }
#ifndef QUIET
    if (remain)
      kissat_phase (solver, "forward", GET (forward_subsumptions),
		    "%zu unchecked clauses remain %.0f%%",
		    remain, kissat_percent (remain, scheduled));
    else
      kissat_phase (solver, "forward", GET (forward_subsumptions),
		    "all %zu scheduled clauses checked", scheduled);
#endif
  }

  kissat_free (solver, saved, solver->vars * sizeof *saved);
  RELEASE_STACK (candidates);

  REPORT (!subsumed, 's');
}

void
kissat_forward_subsume_during_elimination (kissat * solver)
{
  START (forward);
  assert (GET_OPTION (forward));
  INC (forward_subsumptions);
  assert (!solver->watching);
  remove_all_duplicated_binary_clauses (solver);
  if (!solver->inconsistent)
    forward_subsume_all_clauses (solver);
  STOP (forward);
}

static bool
forward_marked_temporary (kissat * solver, unsigned *remove)
{
  const flags *flags = solver->flags;

  for (all_stack (unsigned, lit, solver->clause.lits))
    {
      const unsigned idx = IDX (lit);
      if (!flags[idx].active)
	continue;

      assert (!VALUE (lit));

      if (forward_literal (solver, lit, true, remove))
	return true;
    }

  return false;
}

static bool
forward_subsumed_temporary (kissat * solver)
{
  unsigneds *clause = &solver->clause.lits;

  if (SIZE_STACK (*clause) < 2)
    return false;

  LOGTMP ("trying to forward subsume");

  value *marks = solver->marks;

  for (all_stack (unsigned, lit, *clause))
    {
      assert (!VALUE (lit));
      marks[lit] = 1;
    }

  unsigned remove = INVALID_LIT;
  const bool subsume = forward_marked_temporary (solver, &remove);

  for (all_stack (unsigned, lit, *clause))
      marks[lit] = 0;

  if (subsume)
    {
      INC (subsumed);
      INC (forward_subsumed);
    }
  else if (remove != INVALID_LIT)
    {
      INC (strengthened);
      INC (forward_strengthened);
      unsigned *q = BEGIN_STACK (*clause);
      const unsigned *end = END_STACK (*clause), *p = q;
      while (p != end)
	{
	  const unsigned lit = *p++;
	  if (lit != remove)
	    *q++ = lit;
	}
      assert (q + 1 == end);
      SET_END_OF_STACK (*clause, q);
      LOGTMP ("forward strengthened");
    }

  return subsume;
}

bool
kissat_forward_subsume_temporary (kissat * solver)
{
  assert (!solver->inconsistent);
  if (!GET_OPTION (forward))
    return false;
  START (forward);
  bool res = forward_subsumed_temporary (solver);
  STOP (forward);
  return res;
}
