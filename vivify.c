#include "allocate.h"
#include "backtrack.h"
#include "colors.h"
#include "decide.h"
#include "inline.h"
#include "print.h"
#include "prophyper.h"
#include "proprobe.h"
#include "promote.h"
#include "report.h"
#include "sort.h"
#include "trail.h"
#include "terminate.h"
#include "vivify.h"

#include <inttypes.h>

static inline bool
more_occurrences (unsigned *counts, unsigned a, unsigned b)
{
  const unsigned s = counts[a];
  const unsigned t = counts[b];
  if (s > t)
    return true;
  if (s < t)
    return false;
  return a < b;
}

#define MORE_OCCURRENCES(A,B) \
  more_occurrences (counts, (A), (B))

static void
vivify_sort_lits_by_counts (kissat * solver,
			    size_t size, unsigned *lits, unsigned *counts)
{
  SORT (unsigned, size, lits, MORE_OCCURRENCES);
}

static void
vivify_sort_stack_by_counts (kissat * solver,
			     unsigneds * stack, unsigned *counts)
{
  const size_t size = SIZE_STACK (*stack);
  unsigned *lits = BEGIN_STACK (*stack);
  vivify_sort_lits_by_counts (solver, size, lits, counts);
}

static void
vivify_sort_clause_by_counts (kissat * solver, clause * c, unsigned *counts)
{
  vivify_sort_lits_by_counts (solver, c->size, c->lits, counts);
}

static void
count_literal (unsigned lit, unsigned *counts)
{
  const unsigned old_count = counts[lit];
  const unsigned new_count =
    (old_count < UINT_MAX) ? old_count + 1 : UINT_MAX;
  counts[lit] = new_count;
}

static void
count_clause (clause * c, unsigned *counts)
{
  for (all_literals_in_clause (lit, c))
    count_literal (lit, counts);
}

static bool
simplify_vivification_candidate (kissat * solver, clause * c)
{
  assert (!solver->level);
  bool satisfied = false;
  assert (EMPTY_STACK (solver->clause.lits));
  const value *values = solver->values;
  for (all_literals_in_clause (lit, c))
    {
      const value value = values[lit];
      if (value > 0)
	{
	  satisfied = true;
	  LOGCLS (c, "vivification %s satisfied candidate", LOGLIT (lit));
	  kissat_mark_clause_as_garbage (solver, c);
	  break;
	}
      if (!value)
	PUSH_STACK (solver->clause.lits, lit);
    }
  unsigned non_false = SIZE_STACK (solver->clause.lits);
  if (satisfied)
    {
      CLEAR_STACK (solver->clause.lits);
      return true;
    }
  if (non_false == c->size)
    {
      CLEAR_STACK (solver->clause.lits);
      return false;
    }
  assert (1 < non_false);
  assert (non_false <= c->size);
  if (non_false == 2)
    {
      const unsigned first = PEEK_STACK (solver->clause.lits, 0);
      const unsigned second = PEEK_STACK (solver->clause.lits, 1);
      LOGBINARY (first, second, "vivification shrunken candidate");
      kissat_new_binary_clause (solver, c->redundant, first, second);
      kissat_mark_clause_as_garbage (solver, c);
    }
  else
    {
      CHECK_AND_ADD_STACK (solver->clause.lits);
      ADD_STACK_TO_PROOF (solver->clause.lits);

      REMOVE_CHECKER_CLAUSE (c);
      DELETE_CLAUSE_FROM_PROOF (c);

      const unsigned old_size = c->size;
      unsigned new_size = 0, *lits = c->lits;;
      for (unsigned i = 0; i < old_size; i++)
	{
	  const unsigned lit = lits[i];
	  const value value = kissat_fixed (solver, lit);
	  assert (value <= 0);
	  if (value < 0)
	    continue;
	  lits[new_size++] = lit;
	}
      assert (2 < new_size);
      assert (new_size == non_false);
      assert (new_size < old_size);
      c->size = new_size;
      c->searched = 2;
      if (c->redundant && c->glue >= new_size)
	kissat_promote_clause (solver, c, new_size - 1);
      if (!c->shrunken)
	{
	  c->shrunken = true;
	  lits[old_size - 1] = INVALID_LIT;
	}
      LOGCLS (c, "vivification shrunken candidate");
    }
  CLEAR_STACK (solver->clause.lits);
  return false;
}

static void
schedule_vivification_candidates (kissat * solver,
#ifndef QUIET
				  const char *mode, const char *type,
#endif
				  references * schedule, unsigned *counts,
				  bool redundant, bool tier2)
{
  unsigned lower_glue_limit, upper_glue_limit;
  if (tier2)
    {
      lower_glue_limit = GET_OPTION (tier1) + 1;
      upper_glue_limit = GET_OPTION (tier2);
    }
  else
    {
      lower_glue_limit = 0;
      upper_glue_limit = GET_OPTION (tier1);
    }
  const word *arena = BEGIN_STACK (solver->arena);
  size_t prioritized = 0;
  for (unsigned prioritize = 0; prioritize < 2; prioritize++)
    {
      for (all_clauses (c))
	{
	  if (c->garbage)
	    continue;
	  if (redundant)
	    {
	      if (!c->redundant)
		continue;
	      if (c->hyper)
		continue;
	      if (c->glue < lower_glue_limit)
		continue;
	      if (c->glue > upper_glue_limit)
		continue;
	    }
	  else if (c->redundant)
	    continue;
	  if (c->vivify != prioritize)
	    continue;
	  if (simplify_vivification_candidate (solver, c))
	    continue;
	  if (prioritize)
	    prioritized++;
	  count_clause (c, counts);
	  const reference ref = (word *) c - arena;
	  PUSH_STACK (*schedule, ref);
	}
    }
#ifndef QUIET
  size_t scheduled = SIZE_STACK (*schedule);
#endif
  if (prioritized)
    {
      kissat_phase (solver, mode, GET (probings),
		    "prioritized %zu %s clauses %.0f%%", prioritized,
		    type, kissat_percent (prioritized, scheduled));
    }
  else
    {
      kissat_phase (solver, mode, GET (probings),
		    "prioritizing all %zu scheduled %s clauses",
		    scheduled, type);
      for (all_stack (reference, ref, *schedule))
	{
	  clause *c = (clause *) (arena + ref);
	  assert (kissat_clause_in_arena (solver, c));
	  c->vivify = true;
	}
    }
}

static unsigned *
new_vivification_candidates_counts (kissat * solver)
{
  return kissat_calloc (solver, LITS, sizeof (unsigned));
}

static inline bool
worse_candidate (kissat * solver, unsigned *counts, reference r, reference s)
{
  const clause *c = kissat_dereference_clause (solver, r);
  const clause *d = kissat_dereference_clause (solver, s);

  if (!c->vivify && d->vivify)
    return true;

  if (c->vivify && !d->vivify)
    return false;

  const unsigned *p = BEGIN_LITS (c);
  const unsigned *q = BEGIN_LITS (d);
  const unsigned *e = END_LITS (c);
  const unsigned *f = END_LITS (d);

  while (p != e && q != f)
    {
      const unsigned a = *p++;
      const unsigned b = *q++;
      if (a == b)
	continue;
      const unsigned u = counts[a];
      const unsigned v = counts[b];
      if (u < v)
	return true;
      if (u > v)
	return false;
      return a < b;
    }

  if (p != e && q == f)
    return true;
  if (p == e && q != f)
    return false;

  return r < s;
}

#define WORSE_CANDIDATE(A,B) \
  worse_candidate (solver, counts, (A), (B))

static void
sort_vivification_candidates (kissat * solver,
			      references * schedule, unsigned *counts)
{
  for (all_stack (reference, ref, *schedule))
    {
      clause *c = kissat_dereference_clause (solver, ref);
      vivify_sort_clause_by_counts (solver, c, counts);
    }
  SORT_STACK (reference, *schedule, WORSE_CANDIDATE);
}

static clause *
vivify_unit_conflict (kissat * solver, unsigned lit)
{
  assert (VALUE (lit) > 0);
  LOG ("vivify analyzing conflict unit %s", LOGLIT (NOT (lit)));
  assigned *a = ASSIGNED (lit);
  assert (!a->analyzed);
  assert (a->analyzed != DECISION);
  clause *conflict;
  if (a->binary)
    {
      const unsigned other = a->reason;
      conflict = kissat_binary_conflict (solver, a->redundant, lit, other);
    }
  else
    {
      const reference ref = a->reason;
      conflict = kissat_dereference_clause (solver, ref);
    }
  a->analyzed = true;
  PUSH_STACK (solver->analyzed, lit);
  PUSH_STACK (solver->clause.lits, lit);
  return conflict;
}

static void
vivify_binary_or_large_conflict (kissat * solver, clause * conflict)
{
  assert (conflict->size >= 2);
  LOGCLS (conflict, "vivify analyzing conflict");
  for (all_literals_in_clause (lit, conflict))
    {
      assert (VALUE (lit) < 0);
      assigned *a = ASSIGNED (lit);
      if (!a->level)
	continue;
      assert (!a->analyzed);
      a->analyzed = ANALYZED;
      PUSH_STACK (solver->analyzed, lit);
    }
}

static bool
vivify_analyze (kissat * solver, clause * c,
		clause * conflict, bool * irredundant_ptr)
{
  assert (conflict);
  assert (!EMPTY_STACK (solver->analyzed));

  value *marks = solver->marks;
  for (all_literals_in_clause (lit, c))
    {
      assert (!marks[lit]);
      marks[lit] = 1;
    }

  bool subsumed = false;

  if (c->redundant || !conflict->redundant)
    {
      subsumed = true;
      for (all_literals_in_clause (lit, conflict))
	{
	  const value value = kissat_fixed (solver, lit);
	  if (value < 0)
	    continue;
	  assert (!value);
	  if (marks[lit])
	    continue;
	  subsumed = false;
	  break;
	}
      if (subsumed)
	LOGCLS (conflict, "vivify subsuming");
    }

  size_t analyzed = 0;
  bool irredundant = conflict && !conflict->redundant;

  while (!subsumed && analyzed < SIZE_STACK (solver->analyzed))
    {
      const unsigned not_lit = PEEK_STACK (solver->analyzed, analyzed);
      const unsigned lit = NOT (not_lit);
      analyzed++;
      assigned *a = ASSIGNED (lit);
      assert (a->level);
      assert (a->analyzed);
      if (a->reason == DECISION)
	{
	  LOG ("vivify analyzing decision %s", LOGLIT (not_lit));
	  PUSH_STACK (solver->clause.lits, not_lit);
	}
      else if (a->binary)
	{
	  const unsigned other = a->reason;
	  if (a->redundant)
	    irredundant = false;
	  assert (VALUE (other) < 0);
	  assigned *b = ASSIGNED (other);
	  assert (b->level);
	  if (c->redundant || !a->redundant)
	    {
	      if (marks[lit] && marks[other])
		{
		  LOGBINARY (lit, other, "vivify subsuming");
		  subsumed = true;
		  break;
		}
	    }
	  if (b->analyzed)
	    continue;
	  LOGBINARY (lit, other, "vivify analyzing %s reason", LOGLIT (lit));
	  b->analyzed = ANALYZED;
	  PUSH_STACK (solver->analyzed, other);
	}
      else
	{
	  const reference ref = a->reason;
	  LOGREF (ref, "vivify analyzing %s reason", LOGLIT (lit));
	  clause *reason = kissat_dereference_clause (solver, ref);
	  if (reason->redundant)
	    irredundant = false;
	  subsumed = marks[lit];
	  for (all_literals_in_clause (other, reason))
	    {
	      if (other == lit)
		continue;
	      if (other == not_lit)
		continue;
	      assert (VALUE (other) < 0);
	      assigned *b = ASSIGNED (other);
	      if (!b->level)
		continue;
	      if (!marks[other])
		subsumed = false;
	      if (b->analyzed)
		continue;
	      b->analyzed = ANALYZED;
	      PUSH_STACK (solver->analyzed, other);
	    }
	  if (subsumed && (c->redundant || !reason->redundant))
	    {
	      LOGCLS (reason, "vivify subsuming");
	      break;
	    }
	  subsumed = false;
	}
    }

  for (all_literals_in_clause (lit, c))
    {
      assert (marks[lit]);
      marks[lit] = 0;
    }

  if (!subsumed)
    {
      *irredundant_ptr = irredundant;
      LOGTMP ("vivify learned");
    }

  return subsumed;
}

static void
reset_vivify_analyzed (kissat * solver)
{
  for (all_stack (unsigned, lit, solver->analyzed))
    {
      assigned *a = ASSIGNED (lit);
      assert (a->analyzed);
      a->analyzed = 0;
    }
  CLEAR_STACK (solver->clause.lits);
  CLEAR_STACK (solver->analyzed);
}

static void
vivify_inc_subsume (kissat * solver, clause * c)
{
  if (c->redundant)
    INC (vivify_sub_red);
  else
    INC (vivify_sub_irr);
  INC (vivify_subsumed);
  INC (subsumed);
}

static void
vivify_inc_strengthened (kissat * solver, clause * c)
{
  if (c->redundant)
    INC (vivify_str_red);
  else
    INC (vivify_str_irr);
  INC (vivify_strengthened);
  INC (strengthened);
}

static bool
vivify_learn (kissat * solver, clause * c,
	      unsigned non_false, bool irredundant, unsigned implied)
{
  bool res;

  size_t size = SIZE_STACK (solver->clause.lits);
  assert (size <= non_false);
  assert (2 < non_false);

  if (size < non_false)
    {
      if (solver->level)
	kissat_backtrack (solver, 0);
      LOGTMP ("vivified");
    }

  if (size == 1)
    {
      const unsigned unit = PEEK_STACK (solver->clause.lits, 0);
      kissat_assign_unit (solver, unit);
      solver->iterating = true;
      CHECK_AND_ADD_UNIT (unit);
      ADD_UNIT_TO_PROOF (unit);
      kissat_mark_clause_as_garbage (solver, c);
      clause *conflict = kissat_probing_propagate (solver, 0);
      if (conflict)
	{
	  CHECK_AND_ADD_EMPTY ();
	  ADD_EMPTY_TO_PROOF ();
	  LOG ("propagating vivified unit failed");
	  solver->inconsistent = true;
	}
      else
	{
	  assert (solver->unflushed);
	  kissat_flush_trail (solver);
	}
      vivify_inc_strengthened (solver, c);
      INC (failed);
      res = true;
    }
  else if (size == 2)
    {
      if (c->redundant)
	(void) kissat_new_redundant_clause (solver, 1);
      else
	(void) kissat_new_irredundant_clause (solver);
      kissat_mark_clause_as_garbage (solver, c);
      vivify_inc_strengthened (solver, c);
      res = true;
    }
  else if (size < non_false)
    {
      CHECK_AND_ADD_STACK (solver->clause.lits);
      ADD_STACK_TO_PROOF (solver->clause.lits);

      REMOVE_CHECKER_CLAUSE (c);
      DELETE_CLAUSE_FROM_PROOF (c);

      assert (size > 2);
      const unsigned old_size = c->size;
      unsigned new_size = 0, *lits = c->lits;
      unsigned watched[2] = { lits[0], lits[1] };
      for (unsigned i = 0; i < old_size; i++)
	{
	  const unsigned lit = lits[i];
	  bool keep = true;
	  if (lit != implied)
	    {
	      assigned *a = ASSIGNED (lit);
	      if (!a->analyzed)
		keep = false;
	      else if (a->reason != DECISION)
		keep = false;
	    }
	  if (!c->redundant)
	    {
	      if (keep)
		kissat_mark_added_literal (solver, lit);
	      else
		kissat_mark_removed_literal (solver, lit);
	    }
	  if (keep)
	    lits[new_size++] = lit;
	}
      assert (new_size < old_size);
      assert (new_size == size);
      if (!c->shrunken)
	{
	  c->shrunken = true;
	  lits[old_size - 1] = INVALID_LIT;
	}
      c->size = new_size;
      if (c->redundant && c->glue >= new_size)
	kissat_promote_clause (solver, c, new_size - 1);
      c->searched = 2;
      LOGCLS (c, "vivified shrunken");

      const reference ref = kissat_reference_clause (solver, c);

      // Beware of 'stale blocking literals' ... so rewatch if shrunken.

      kissat_unwatch_blocking (solver, watched[0], ref);
      kissat_unwatch_blocking (solver, watched[1], ref);
      kissat_watch_blocking (solver, lits[0], lits[1], ref);
      kissat_watch_blocking (solver, lits[1], lits[0], ref);

      vivify_inc_strengthened (solver, c);
      res = true;
    }
  else if (irredundant && !c->redundant)
    {
      LOGCLS (c, "vivify subsumed");
      vivify_inc_subsume (solver, c);
      kissat_mark_clause_as_garbage (solver, c);
      res = true;
    }
  else
    {
      LOG ("vivify failed");
      res = false;
    }

  return res;
}

static bool
vivify_clause (kissat * solver, clause * c,
	       unsigneds * sorted, unsigned *counts)
{
  assert (!c->garbage);
  assert (solver->probing);
  assert (solver->watching);
  assert (!solver->inconsistent);

  LOGCLS (c, "vivify candidate");

  CLEAR_STACK (*sorted);

  for (all_literals_in_clause (lit, c))
    {
      const value value = kissat_fixed (solver, lit);
      if (value < 0)
	continue;
      if (value > 0)
	{
	  LOGCLS (c, "%s satisfied", LOGLIT (lit));
	  kissat_mark_clause_as_garbage (solver, c);
	  break;
	}
      PUSH_STACK (*sorted, lit);
    }

  if (c->garbage)
    return false;

  const unsigned non_false = SIZE_STACK (*sorted);

  assert (1 < non_false);
  assert (non_false <= c->size);

#ifdef LOGGING
  if (!non_false)
    LOG ("no root level falsified literal");
  else if (non_false == c->size)
    LOG ("all literals root level unassigned");
  else
    LOG ("found %u root level non-falsified literals");
#endif

  if (non_false == 2)
    {
      LOGCLS (c, "skipping actually binary");
      return false;
    }

  INC (vivification_checks);

  unsigned unit = INVALID_LIT;
  for (all_literals_in_clause (lit, c))
    {
      const value value = VALUE (lit);
      if (value < 0)
	continue;
      if (!value)
	{
	  unit = INVALID_LIT;
	  break;
	}
      assert (value > 0);
      if (unit != INVALID_LIT)
	{
	  unit = INVALID_LIT;
	  break;
	}
      unit = lit;
    }
  if (unit != INVALID_LIT)
    {
      assigned *a = ASSIGNED (unit);
      assert (a->level);
      if (a->binary)
	unit = INVALID_LIT;
      else
	{
	  reference ref = kissat_reference_clause (solver, c);
	  if (a->reason != ref)
	    unit = INVALID_LIT;
	}
    }
  if (unit == INVALID_LIT)
    LOG ("non-reason candidate clause");
  else
    {
      LOG ("candidate is the reason of %s", LOGLIT (unit));
      const unsigned level = LEVEL (unit);
      assert (level > 0);
      LOG ("forced to backtrack to level %u", level - 1);
      kissat_backtrack (solver, level - 1);
    }

  assert (EMPTY_STACK (solver->analyzed));
  assert (EMPTY_STACK (solver->clause.lits));

  vivify_sort_stack_by_counts (solver, sorted, counts);

#if defined(LOGGING) && !defined(NOPTIONS)
  if (solver->options.log)
    {
      TERMINAL (stdout, 1);
      COLOR (MAGENTA);
      printf ("c LOG %u vivify sorted", solver->level);
      heap *scores = &solver->scores;
      links *links = solver->links;
      for (all_stack (unsigned, lit, *sorted))
	{
	  printf (" %s", LOGLIT (lit));
	  if (counts)
	    printf ("#%u", counts[lit]);
	  else
	    {
	      const unsigned idx = IDX (lit);
	      if (solver->stable)
		printf ("[%g]", kissat_get_heap_score (scores, idx));
	      else
		printf ("{%u}", links[idx].stamp);
	    }

	}
      COLOR (NORMAL);
      printf ("\n");
      fflush (stdout);
    }
#endif

  unsigned implied = INVALID_LIT;
  clause *conflict = 0;
  unsigned level = 0;
  bool res = false;

  for (all_stack (unsigned, lit, *sorted))
    {
      if (level++ < solver->level)
	{
	  frame *frame = &FRAME (level);
	  const unsigned not_lit = NOT (lit);
	  if (frame->decision == not_lit)
	    {
	      LOG ("reusing assumption %s", LOGLIT (not_lit));
	      INC (vivify_reused);
	      INC (vivify_probes);
	      assert (VALUE (lit) < 0);
	      continue;
	    }

	  LOG ("forced to backtrack to decision level %u", level - 1);
	  kissat_backtrack (solver, level - 1);
	}

      const value value = VALUE (lit);
      assert (!value || LEVEL (lit) <= level);

      if (!value)
	{
	  LOG ("literal %s unassigned", LOGLIT (lit));
	  const unsigned not_lit = NOT (lit);
	  INC (vivify_assumed);
	  INC (vivify_probes);
	  kissat_internal_assume (solver, not_lit);
	  if (solver->level == 1)
	    conflict = kissat_hyper_propagate (solver, c);
	  else
	    conflict = kissat_probing_propagate (solver, c);
	  if (!conflict)
	    continue;
	  vivify_binary_or_large_conflict (solver, conflict);
	  assert (!EMPTY_STACK (solver->analyzed));
	  break;
	}

      if (value < 0)
	{
	  assert (LEVEL (lit));
	  LOG ("literal %s already falsified", LOGLIT (lit));
	  continue;
	}

      assert (value > 0);
      assert (LEVEL (lit));
      LOG ("literal %s already satisfied", LOGLIT (lit));
      if (!c->redundant || GET_OPTION (vivifyimply) == 1)
	{
	  implied = lit;
	  conflict = vivify_unit_conflict (solver, lit);
	  assert (!EMPTY_STACK (solver->analyzed));
	}
      else if (GET_OPTION (vivifyimply) == 2)
	{
	  LOGCLS (c, "vivify implied");
	  kissat_mark_clause_as_garbage (solver, c);
	  INC (vivify_implied);
	  res = true;
	}
      break;
    }

  if (c->garbage)
    assert (EMPTY_STACK (solver->analyzed));
  else if (conflict)
    {
      assert (!EMPTY_STACK (solver->analyzed));
      assert (solver->level);
      bool irredundant;
      const bool subsumed =
	vivify_analyze (solver, c, conflict, &irredundant);

      if (subsumed)
	{
	  LOGCLS (c, "vivify subsumed");
	  kissat_mark_clause_as_garbage (solver, c);
	  vivify_inc_subsume (solver, c);
	  res = true;
	}
      else
	res = vivify_learn (solver, c, non_false, irredundant, implied);

      reset_vivify_analyzed (solver);
    }
  else
    {
      assert (EMPTY_STACK (solver->analyzed));
      LOG ("vivify failed");
    }

  if (!res)
    return false;

  INC (vivified);
  if (c->redundant)
    INC (vivify_redundant);
  else
    INC (vivify_irredundant);

  return true;
}

enum round
{
  REDUNDANT_TIER1_ROUND = 1,
  REDUNDANT_TIER2_ROUND = 2,
  IRREDUNDANT_ROUND = 3
};

static void
vivify_round (kissat * solver, enum round round)
{
  assert (solver->watching);
  assert (solver->probing);

#ifndef QUIET
  const char *mode;
  const char *type;
  char tag;
  if (round == REDUNDANT_TIER2_ROUND)
    {
      mode = "vivify-redundant-tier2";
      type = "redundant";
      tag = 'u';
    }
  else if (round == REDUNDANT_TIER1_ROUND)
    {
      mode = "vivify-redundant-tier1";
      type = "redundant";
      tag = 'v';
    }
  else
    {
      assert (round == IRREDUNDANT_ROUND);
      mode = "vivify-irredundant";
      type = "irredundant";
      tag = 'w';
    }
#endif

  references schedule;
  INIT_STACK (schedule);

  unsigned *counts = 0;
  kissat_flush_large_watches (solver);
  counts = new_vivification_candidates_counts (solver);
  {
    bool redundant, tier2;

    if (round == REDUNDANT_TIER1_ROUND)
      redundant = true, tier2 = false;
    else if (round == REDUNDANT_TIER2_ROUND)
      redundant = tier2 = true;
    else
      {
	assert (round == IRREDUNDANT_ROUND);
	redundant = tier2 = false;
      }

    schedule_vivification_candidates (solver,
#ifndef QUIET
				      mode, type,
#endif
				      &schedule, counts, redundant, tier2);
  }
  sort_vivification_candidates (solver, &schedule, counts);
  kissat_watch_large_clauses (solver);

  const size_t scheduled = SIZE_STACK (schedule);
#ifndef QUIET
  const size_t total =
    (round == IRREDUNDANT_ROUND) ? IRREDUNDANT_CLAUSES : REDUNDANT_CLAUSES;
  kissat_phase (solver, mode, GET (probings),
		"scheduled %zu %s clauses %.0f%% of %zu", scheduled,
		type, kissat_percent (scheduled, total), total);
#endif
  SET_EFFICIENCY_BOUND (ticks_limit, vivify,
			probing_ticks, search_ticks,
			kissat_nlogn (scheduled));

  if (round == REDUNDANT_TIER2_ROUND)
    {
      const uint64_t delta = ticks_limit - solver->statistics.probing_ticks;
      ticks_limit += 3 * delta;
      kissat_very_verbose (solver,
			   "increasing redundant tier2 efficiency limit to %"
			   PRIu64 " by %" PRIu64 " = 3 * %" PRIu64,
			   ticks_limit, 3 * delta, delta);
    }

  size_t vivified = 0, tried = 0;
  unsigneds sorted;
  INIT_STACK (sorted);
  while (!EMPTY_STACK (schedule))
    {
      if (solver->statistics.probing_ticks > ticks_limit)
	break;
      if (TERMINATED (19))
	break;
      const reference ref = POP_STACK (schedule);
      clause *c = kissat_dereference_clause (solver, ref);
      if (c->garbage)
	continue;
      tried++;
      if (vivify_clause (solver, c, &sorted, counts))
	vivified++;
      c->vivify = false;
      if (solver->inconsistent)
	break;
    }
  if (solver->level)
    kissat_backtrack (solver, 0);
  kissat_dealloc (solver, counts, LITS, sizeof *counts);
  RELEASE_STACK (sorted);
#ifndef QUIET
  kissat_phase (solver, mode, GET (probings),
		"vivified %zu %s clauses %.0f%% out of %zu tried",
		vivified, type, kissat_percent (vivified, tried), tried);
  if (!solver->inconsistent)
    {
      size_t remain = SIZE_STACK (schedule);
      if (remain)
	{
	  kissat_phase (solver, mode, GET (probings),
			"%zu %s clauses remain %.0f%% out of %zu scheduled",
			remain, type, kissat_percent (remain, scheduled),
			scheduled);

	  const word *arena = BEGIN_STACK (solver->arena);
	  size_t prioritized = 0;
	  while (!EMPTY_STACK (schedule))
	    {
	      const unsigned ref = POP_STACK (schedule);
	      clause *c = (clause *) (arena + ref);
	      if (c->vivify)
		prioritized++;
	    }
	  if (prioritized)
	    kissat_phase (solver, mode, GET (probings),
			  "keeping %zu %s clauses prioritized %.0f%%",
			  prioritized, type,
			  kissat_percent (prioritized, remain));
	  else
	    kissat_phase (solver, mode, GET (probings),
			  "no prioritized %s clauses left", type);
	}
      else
	kissat_phase (solver, mode, GET (probings),
		      "all scheduled %s clauses tried", type);
    }
#endif
  RELEASE_STACK (schedule);
  REPORT (!vivified, tag);
}

static void
vivify_redundant_tier1 (kissat * solver)
{
  if (TERMINATED (20))
    return;
  vivify_round (solver, REDUNDANT_TIER1_ROUND);
}

static void
vivify_redundant_tier2 (kissat * solver)
{
  if (TERMINATED (21))
    return;
  vivify_round (solver, REDUNDANT_TIER2_ROUND);
}

static void
vivify_irredundant (kissat * solver)
{
  if (TERMINATED (22))
    return;
  vivify_round (solver, IRREDUNDANT_ROUND);
}

static bool
really_vivify (kissat * solver)
{
  if (!GET_OPTION (really))
    return true;
  const uint64_t limit = IRREDUNDANT_CLAUSES + 5 * REDUNDANT_CLAUSES;
  statistics *statistics = &solver->statistics;
  const uint64_t visits = statistics->search_ticks;
  return limit < visits + GET_OPTION (vivifymineff);
}

void
kissat_vivify (kissat * solver)
{
  if (solver->inconsistent)
    return;
  assert (!solver->level);
  assert (solver->probing);
  assert (solver->watching);
  if (!GET_OPTION (vivify))
    return;
  if (!solver->statistics.clauses_redundant)
    return;
  if (!really_vivify (solver))
    return;
  START (vivify);
  vivify_redundant_tier2 (solver);
  if (!solver->inconsistent)
    {
      vivify_redundant_tier1 (solver);
      if (!solver->inconsistent &&
	  IRREDUNDANT_CLAUSES / 10 < REDUNDANT_CLAUSES)
	vivify_irredundant (solver);
    }
  STOP (vivify);
}
