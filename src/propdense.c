#define INLINE_ASSIGN

#include "inline.h"
#include "propdense.h"

// Keep this 'inlined' file separate:

#include "assign.c"

static inline bool
non_watching_propagate_literal (kissat * solver,
				unsigned lit, unsigned ignore)
{
  assert (IDX (lit) != ignore);

  assert (!solver->watching);
  LOG ("propagating %s", LOGLIT (lit));
  assert (VALUE (lit) > 0);
  const unsigned not_lit = NOT (lit);

  watches *watches = &WATCHES (not_lit);
  unsigned ticks = 1 + kissat_cache_lines (watches->size, sizeof (watch));

  const word *arena = BEGIN_STACK (solver->arena);
  assigned *assigned = solver->assigned;
  value *values = solver->values;

  const unsigned level = solver->level;

  for (all_binary_large_watches (watch, *watches))
    {
      if (watch.type.binary)
	{
	  const unsigned other = watch.binary.lit;
	  const unsigned other_idx = IDX (other);
	  if (other_idx == ignore)
	    continue;
	  assert (VALID_INTERNAL_LITERAL (other));
	  const value other_value = values[other];
	  if (other_value > 0)
	    continue;
	  if (other_value < 0)
	    {
	      LOGBINARY (not_lit, other, "conflicting");
	      return false;
	    }
	  const bool redundant = watch.binary.redundant;
	  kissat_assign_binary (solver, values, assigned, redundant, other,
				not_lit);
	}
      else
	{
	  const reference ref = watch.large.ref;
	  assert (ref < SIZE_STACK (solver->arena));
	  clause *c = (clause *) (arena + ref);
	  assert (c->size > 2);
	  assert (!c->redundant);
	  ticks++;
	  if (c->garbage)
	    continue;
	  unsigned non_false = 0;
	  unsigned unit = INVALID_LIT;
	  bool satisfied = false;
	  bool ignored = false;
	  for (all_literals_in_clause (other, c))
	    {
	      if (other == not_lit)
		continue;
	      const unsigned other_idx = IDX (other);
	      if (other_idx == ignore)
		{
		  ignored = true;
		  break;
		}
	      assert (VALID_INTERNAL_LITERAL (other));
	      const value other_value = values[other];
	      if (other_value < 0)
		continue;
	      if (other_value > 0)
		{
		  satisfied = true;
		  if (!level)
		    {
		      LOGCLS (c, "%s satisfied", LOGLIT (other));
		      kissat_mark_clause_as_garbage (solver, c);
		    }
		  break;
		}
	      if (!non_false++)
		unit = other;
	      else if (non_false > 1)
		break;
	    }
	  if (ignored)
	    continue;
	  if (satisfied)
	    continue;
	  if (!non_false)
	    {
	      LOGREF (ref, "conflicting");
	      return false;
	    }
	  if (non_false == 1)
	    kissat_assign_reference (solver, values, assigned, unit, ref, c);
	}
    }

  ADD (ticks, ticks);
  ADD (dense_ticks, ticks);

  return true;
}

bool
kissat_dense_propagate (kissat * solver, unsigned limit, unsigned ignore_idx)
{
  assert (!solver->probing);
  assert (!solver->watching);
  assert (!solver->inconsistent);
  START (propagate);
  unsigned end_of_last_generation = SIZE_STACK (solver->trail);
  unsigned generations = 0;
  unsigned propagated = 0;
  bool res = true;
  while (res && generations < limit &&
	 solver->propagated < SIZE_STACK (solver->trail))
    {
      const unsigned lit = PEEK_STACK (solver->trail, solver->propagated);
      res = non_watching_propagate_literal (solver, lit, ignore_idx);
      if (++solver->propagated == end_of_last_generation)
	{
	  LOG ("propagated generation %u", generations);
	  end_of_last_generation = SIZE_STACK (solver->trail);
	  generations++;
	}
      propagated++;
    }
  LOG ("%s %u literals in %u generations %s conflict",
       (solver->propagated == SIZE_STACK (solver->trail)) ?
       "completely propagated" : "incomplete propagation of",
       propagated, generations, res ? "without" : "with");
  ADD (dense_propagations, propagated);
  ADD (propagations, propagated);
  STOP (propagate);
  return res;
}
