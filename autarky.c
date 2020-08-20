#include "allocate.h"
#include "autarky.h"
#include "decide.h"
#include "dense.h"
#include "inline.h"
#include "print.h"
#include "report.h"
#include "terminate.h"
#include "weaken.h"

static inline void
autarky_unassign (kissat * solver,
		  value * autarky, unsigneds * work, unsigned lit)
{
  const unsigned not_lit = NOT (lit);
  assert (autarky[lit] > 0);
  LOG ("autarky unassign %s", LOGLIT (lit));
  autarky[lit] = autarky[not_lit] = 0;
  if (work)
    PUSH_STACK (*work, lit);
}

static inline unsigned
propagate_clause (kissat * solver,
		  const value * values, value * autarky,
		  unsigneds * work, clause * c)
{
  assert (!c->garbage);
  assert (!c->redundant);
  unsigned unassigned = 0;
  bool satisfied = false, falsified = false;
  for (all_literals_in_clause (other, c))
    {
      value other_value = values[other];
      if (other_value > 0)
	{
	  LOGCLS (c, "%s satisfied", LOGLIT (other));
	  kissat_mark_clause_as_garbage (solver, c);
	  return 0;
	}
      if (other_value < 0)
	continue;
      other_value = autarky[other];
      if (other_value > 0)
	satisfied = true;
      else if (other_value < 0)
	falsified = true;
    }
  if (satisfied)
    return 0;
  if (!falsified)
    return 0;
  for (all_literals_in_clause (other, c))
    {
      value other_value = values[other];
      if (other_value < 0)
	continue;
      assert (!other_value);
      other_value = autarky[other];
      if (!other_value)
	continue;
      assert (other_value < 0);
      autarky_unassign (solver, autarky, work, NOT (other));
      assert (unassigned < UINT_MAX);
      unassigned++;
    }
  return unassigned;
}

static inline unsigned
propagate_unassigned (kissat * solver, word * arena,
		      const value * values, value * autarky,
		      unsigneds * work, unsigned lit)
{
  assert (!autarky[lit]);
  assert (!solver->watching);
  assert (!solver->level);
  LOG ("autarky propagate unassigned %s", LOGLIT (lit));
  unsigned unassigned = 0;
  watches *watches = &WATCHES (lit);
  watch *begin = BEGIN_WATCHES (*watches), *q = begin;
  const watch *end = END_WATCHES (*watches), *p = q;
  while (p != end)
    {
      const watch watch = *p++;
      if (watch.type.binary)
	{
	  *q++ = watch;
	  assert (!watch.binary.redundant);
	  const unsigned other = watch.binary.lit;
	  value other_value = values[other];
	  if (other_value > 0)
	    continue;
	  assert (!other_value);
	  other_value = autarky[other];
	  if (other_value >= 0)
	    continue;
	  assert (other_value < 0);
	  autarky_unassign (solver, autarky, work, NOT (other));
	  assert (unassigned < UINT_MAX);
	  unassigned++;
	}
      else
	{
	  const reference ref = watch.large.ref;
	  clause *c = (clause *) (arena + ref);
	  assert (kissat_clause_in_arena (solver, c));
	  unassigned += propagate_clause (solver, values, autarky, work, c);
	}
    }
  SET_END_OF_WATCHES (*watches, q);
  return unassigned;
}

static inline unsigned
propagate_autarky (kissat * solver,
		   const value * values, value * autarky, unsigneds * work)
{
  unsigned unassigned = 0;
  word *arena = BEGIN_STACK (solver->arena);
  while (!EMPTY_STACK (*work))
    {
      unsigned lit = POP_STACK (*work);
      unassigned += propagate_unassigned (solver, arena,
					  values, autarky, work, lit);
    }
  return unassigned;
}

static unsigned
determine_autarky (kissat * solver, value * autarky, unsigneds * work)
{
  assert (!solver->level);
  const phase *phases = solver->phases;
  unsigned assigned = 0;
  for (all_variables (idx))
    {
      if (!ACTIVE (idx))
	continue;
      value value = phases[idx].saved;
      if (!value)
	value = INITIAL_PHASE;
      const unsigned lit = LIT (idx);
      const unsigned not_lit = NOT (lit);
      autarky[lit] = value;
      autarky[not_lit] = -value;
      LOG ("autarky assign %s", LOGLIT (value < 0 ? not_lit : lit));
      assigned++;
    }

  const value *values = solver->values;

  clause *last_irredundant = kissat_last_irredundant_clause (solver);

  for (all_clauses (c))
    {
      if (last_irredundant && c > last_irredundant)
	break;
      if (c->garbage)
	continue;
      if (c->redundant)
	continue;
      if (TERMINATED (0))
	return UINT_MAX;
      unsigned unassigned = propagate_clause (solver, values, autarky, 0, c);
      if (!unassigned)
	continue;
      assert (assigned >= unassigned);
      assigned -= unassigned;
      if (!assigned)
	break;
    }

  if (assigned)
    kissat_phase (solver, "autarky", GET (autarky_determined),
		  "preliminary large clause autarky of size %u", assigned);
  else
    {
      kissat_phase (solver, "autarky", GET (autarky_determined),
		    "empty preliminary large clause autarky");
      return UINT_MAX;
    }

  for (all_literals (lit))
    {
      if (!assigned)
	break;
      if (!ACTIVE (IDX (lit)))
	continue;
      value lit_value = autarky[lit];
      if (lit_value > 0)
	continue;
      if (TERMINATED (1))
	return UINT_MAX;
      watches *watches = &WATCHES (lit);
      for (all_binary_large_watches (watch, *watches))
	{
	  assert (watch.type.binary);
	  assert (!watch.binary.redundant);
	  const unsigned other = watch.binary.lit;
	  assert (!values[other]);
	  const value other_value = autarky[other];
	  if (other_value > 0)
	    continue;
	  if (lit_value)
	    {
	      assert (lit_value < 0);
	      autarky_unassign (solver, autarky, work, NOT (lit));
	      lit_value = 0;
	      assert (assigned > 0);
	      if (!--assigned)
		break;
	    }
	  if (!other_value)
	    continue;
	  assert (other_value < 0);
	  autarky_unassign (solver, autarky, work, NOT (other));
	  assert (assigned > 0);
	  if (!--assigned)
	    break;
	}
      if (EMPTY_STACK (*work))
	continue;
      const unsigned unassigned =
	propagate_autarky (solver, values, autarky, work);
      assert (unassigned <= assigned);
      assigned -= unassigned;
    }

  if (assigned)
    kissat_phase (solver, "autarky", GET (autarky_determined),
		  "preliminary binary clause autarky of size %u", assigned);
  else
    {
      kissat_phase (solver, "autarky", GET (autarky_determined),
		    "empty preliminary binary clause autarky");
      return UINT_MAX;
    }

  for (all_clauses (c))
    {
      if (last_irredundant && c > last_irredundant)
	break;
      if (c->garbage)
	continue;
      if (c->redundant)
	continue;
      if (TERMINATED (2))
	return UINT_MAX;
      unsigned unassigned =
	propagate_clause (solver, values, autarky, work, c);
      if (unassigned)
	{
	  unassigned += propagate_autarky (solver, values, autarky, work);
	  assert (unassigned <= assigned);
	  assigned -= unassigned;
	  if (!assigned)
	    break;
	}
      else if (!c->garbage)
	{
	  reference ref = INVALID_REF;
	  for (all_literals_in_clause (lit, c))
	    {
	      value lit_value = values[lit];
	      if (lit_value < 0)
		continue;
	      assert (!lit_value);
	      lit_value = autarky[lit];
	      if (lit_value <= 0)
		continue;
	      if (ref == INVALID_REF)
		ref = kissat_reference_clause (solver, c);
	      kissat_connect_literal (solver, lit, ref);
	    }
	}
    }

  if (assigned)
    kissat_phase (solver, "autarky", GET (autarky_determined),
		  "final autarky of size %u", assigned);
  else
    kissat_phase (solver, "autarky", GET (autarky_determined),
		  "empty final autarky");

  return assigned;
}

static void
flush_large_connected_and_autarky_binaries (kissat * solver)
{
  assert (!solver->watching);
  LOG ("flushing large connected clause references and autarky binaries");
  size_t flushed_large = 0, flushed_binaries = 0;
  const flags *flags = solver->flags;
  for (all_literals (lit))
    {
      watches *watches = &WATCHES (lit);
      watch *begin = BEGIN_WATCHES (*watches), *q = begin;
      const watch *end_watches = END_WATCHES (*watches), *p = q;
      const unsigned lit_idx = IDX (lit);
      const struct flags *lit_flags = flags + lit_idx;
      const bool lit_eliminated = lit_flags->eliminated;
      while (p != end_watches)
	{
	  const watch watch = *p++;
	  if (!watch.type.binary)
	    {
	      flushed_large++;
	      continue;
	    }
	  const unsigned other = watch.binary.lit;
	  const unsigned idx_other = IDX (other);
	  const struct flags *other_flags = flags + idx_other;
	  const bool other_eliminated = other_flags->eliminated;
	  if (!lit_eliminated && !other_eliminated)
	    *q++ = watch;
	  else if (lit < other)
	    {
	      assert (!watch.binary.redundant);
	      kissat_delete_binary (solver, false, false, lit, other);
	      flushed_binaries++;
	    }
	}
      SET_END_OF_WATCHES (*watches, q);
    }
#ifndef QUIET
  if (flushed_large)
    kissat_very_verbose (solver, "flushed %zu large clause references",
			 flushed_large);
  if (flushed_binaries)
    kissat_very_verbose (solver, "flushed %zu autarky binary clauses",
			 flushed_binaries);
#else
  (void) flushed_binaries;
  (void) flushed_large;
#endif
}

static void
autarky_literal (kissat * solver, unsigned lit)
{
  LOG ("autarky literal %s", LOGLIT (lit));
  INC (autarky);
  kissat_mark_eliminated_variable (solver, IDX (lit));
  word *arena = BEGIN_STACK (solver->arena);
  if (GET_OPTION (incremental))
    {
      watches *watches = &WATCHES (lit);
      watch *q = BEGIN_WATCHES (*watches);
      const watch *end = END_WATCHES (*watches);
      const watch *p = q;
      while (p != end)
	{
	  const watch watch = *p++;
	  if (watch.type.binary)
	    {
	      assert (!watch.binary.redundant);
	      const unsigned other = watch.binary.lit;
	      kissat_weaken_binary (solver, lit, other);
	      *q++ = watch;
	    }
	  else
	    {
	      const reference ref = watch.large.ref;
	      clause *c = (clause *) (arena + ref);
	      assert (kissat_clause_in_arena (solver, c));
	      assert (!c->redundant);
	      if (!c->garbage)
		{
		  kissat_weaken_clause (solver, lit, c);
		  kissat_mark_clause_as_garbage (solver, c);
		}
	    }
	}
      SET_END_OF_WATCHES (*watches, q);
    }
  else
    kissat_weaken_unit (solver, lit);
}

static void
apply_autarky (kissat * solver, unsigned size, value * autarky)
{
  unsigned eliminated = 0;
  for (all_variables (idx))
    {
      unsigned lit = LIT (idx);
      const value value = autarky[lit];
      if (!value)
	continue;
      if (value < 0)
	lit = NOT (lit);
      assert (autarky[lit] > 0);
      autarky_literal (solver, lit);
      eliminated++;
    }
  LOG ("eliminated %u autarky variables", eliminated);
  assert (eliminated == size);
  (void) eliminated;
  (void) size;
}

void
kissat_autarky (kissat * solver)
{
  if (solver->inconsistent)
    return;
  if (TERMINATED (3))
    return;
  if (!solver->enabled.autarky)
    return;
  RETURN_IF_DELAYED (autarky);
  assert (solver->watching);
  assert (!solver->level);
  STOP_SEARCH_AND_START_SIMPLIFIER (autarky);
  INC (autarky_determined);
  litwatches saved;
  INIT_STACK (saved);
  kissat_enter_dense_mode (solver, 0, &saved);
  solver->watching = false;
  value *autarky = kissat_calloc (solver, LITS, sizeof (value));
  unsigneds work;
  INIT_STACK (work);
  unsigned autarkic = determine_autarky (solver, autarky, &work);
  RELEASE_STACK (work);
  if (autarkic && autarkic < UINT_MAX)
    {
      apply_autarky (solver, autarkic, autarky);
      flush_large_connected_and_autarky_binaries (solver);
    }
  else if (autarkic != UINT_MAX)
    kissat_flush_large_connected (solver);
  kissat_dealloc (solver, autarky, LITS, sizeof (value));
  kissat_resume_sparse_mode (solver, true, 0, &saved);
  RELEASE_STACK (saved);
  bool success = (autarkic && autarkic < UINT_MAX);
  UPDATE_DELAY (success, autarky);
  REPORT (!success, 'a');
  STOP_SIMPLIFIER_AND_RESUME_SEARCH (autarky);
  kissat_check_statistics (solver);
}
