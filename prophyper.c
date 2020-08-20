#define INLINE_ASSIGN

#include "inline.h"
#include "dominate.h"
#include "prophyper.h"

// Keep this 'inlined' file separate:

#include "assign.c"

static inline void
watch_hyper_delayed (kissat * solver, unsigneds * delayed)
{
  const unsigned *end_delayed = END_STACK (*delayed);
  const unsigned *d = BEGIN_STACK (*delayed);
  watches *all_watches = solver->watches;
  while (d != end_delayed)
    {
      const unsigned lit = *d++;
      assert (d != end_delayed);
      const watch watch = {.raw = *d++ };
      watches *lit_watches = all_watches + lit;
      if (watch.type.binary)
	{
	  assert (solver->probing);
	  assert (watch.binary.hyper);
	  assert (watch.binary.redundant);
	  LOGBINARY (lit, watch.binary.lit,
		     "watching blocking %s in %s",
		     LOGLIT (lit), LOGLIT (watch.binary.lit));
	  assert (lit < LITS);
	  PUSH_WATCHES (*lit_watches, watch);
	}
      else
	{
	  assert (d != end_delayed);
	  const reference ref = *d++;
	  const unsigned blocking = watch.blocking.lit;
	  LOGREF (ref, "watching %s blocking %s in", LOGLIT (lit),
		  LOGLIT (blocking));
	  kissat_push_blocking_watch (solver, lit_watches, blocking, ref);
	}
    }
  CLEAR_STACK (*delayed);
}

static inline void
delay_watching_hyper (kissat * solver, unsigneds * delayed,
		      unsigned lit, unsigned other)
{
  const watch watch = kissat_binary_watch (other, true, true);
  PUSH_STACK (*delayed, lit);
  PUSH_STACK (*delayed, watch.raw);
}

#define HYPER_PROPAGATION
#define PROPAGATE_LITERAL large_propagate_literal
#define PROPAGATION_TYPE "large"

#include "proplit.h"

static inline clause *
binary_propagate_literal (kissat * solver, unsigned lit)
{
  assert (solver->probing);
  assert (solver->watching);
  assert (solver->level == 1);

  LOG ("binary propagating %s", LOGLIT (lit));
  assert (VALUE (lit) > 0);
  const unsigned not_lit = NOT (lit);

  watches *watches = &WATCHES (not_lit);
  value *values = solver->values;
  assigned *assigned = solver->assigned;

  const watch *begin = BEGIN_WATCHES (*watches);
  const watch *end = END_WATCHES (*watches);
  const watch *p = begin;

  clause *res = 0;

  while (p != end)
    {
      watch head = *p++;
      if (!head.type.binary)
	{
	  p++;
	  continue;
	}
      const unsigned other = head.binary.lit;
      assert (VALID_INTERNAL_LITERAL (other));
      const value other_value = values[other];
      if (other_value > 0)
	continue;
      const bool redundant = head.binary.redundant;
      if (other_value < 0)
	{
	  res = kissat_binary_conflict (solver, redundant, not_lit, other);
	  break;
	}
      assert (!other_value);
      kissat_assign_binary (solver, values, assigned, redundant, other,
			    not_lit);
    }
  return res;
}

static inline clause *
hyper_propagate (kissat * solver, const clause * ignore)
{
  assert (solver->probing);
  assert (solver->watching);
  assert (solver->level == 1);
  clause *res = 0;
  unsigned binary_propagated = solver->propagated;
  while (!res && solver->propagated < SIZE_STACK (solver->trail))
    {
      assert (solver->propagated <= binary_propagated);
      if (binary_propagated < SIZE_STACK (solver->trail))
	{
	  const unsigned lit = PEEK_STACK (solver->trail, binary_propagated);
	  res = binary_propagate_literal (solver, lit);
	  binary_propagated++;
	}
      else
	{
	  const unsigned lit = PEEK_STACK (solver->trail, solver->propagated);
	  res = large_propagate_literal (solver, ignore, lit);
	  solver->propagated++;
	}
    }
  return res;
}

clause *
kissat_hyper_propagate (kissat * solver, const clause * ignore)
{
  assert (solver->probing);
  assert (solver->watching);
  assert (!solver->inconsistent);
  assert (solver->level == 1);
  assert (!solver->unflushed);

  START (propagate);

  solver->ticks = 0;
  const unsigned propagated = solver->propagated;
  clause *conflict = hyper_propagate (solver, ignore);
  kissat_update_probing_propagation_statistics (solver, propagated);

  STOP (propagate);

  return conflict;
}
