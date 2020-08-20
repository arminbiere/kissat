#include "allocate.h"
#include "analyze.h"
#include "internal.h"
#include "logging.h"
#include "print.h"
#include "proprobe.h"
#include "report.h"
#include "terminate.h"
#include "trail.h"
#include "transitive.h"

#include <stddef.h>

static void
transitive_assign (kissat * solver, unsigned lit)
{
  LOG ("transitive assign %s", LOGLIT (lit));
  value *values = solver->values;
  const unsigned not_lit = NOT (lit);
  assert (!values[lit]);
  assert (!values[not_lit]);
  values[lit] = 1;
  values[not_lit] = -1;
  PUSH_STACK (solver->trail, lit);
}

static void
transitive_backtrack (kissat * solver, unsigned saved)
{
  assert (saved <= SIZE_STACK (solver->trail));
  value *values = solver->values;
  while (SIZE_STACK (solver->trail) > saved)
    {
      const unsigned lit = POP_STACK (solver->trail);
      LOG ("transitive unassign %s", LOGLIT (lit));
      const unsigned not_lit = NOT (lit);
      assert (values[lit] > 0);
      assert (values[not_lit] < 0);
      values[lit] = values[not_lit] = 0;
    }
  solver->propagated = saved;
  solver->level = 0;
}

static void
prioritize_binaries (kissat * solver)
{
  assert (solver->watching);
  statches large;
  INIT_STACK (large);
  watches *all_watches = solver->watches;
  for (all_literals (lit))
    {
      assert (EMPTY_STACK (large));
      watches *watches = all_watches + lit;
      watch *begin_watches = BEGIN_WATCHES (*watches), *q = begin_watches;
      const watch *end_watches = END_WATCHES (*watches), *p = q;
      while (p != end_watches)
	{
	  const watch head = *q++ = *p++;
	  if (head.type.binary)
	    continue;
	  const watch tail = *p++;
	  PUSH_STACK (large, head);
	  PUSH_STACK (large, tail);
	  q--;
	}
      const watch *end_large = END_STACK (large);
      const watch *r = BEGIN_STACK (large);
      while (r != end_large)
	*q++ = *r++;
      assert (q == end_watches);
      CLEAR_STACK (large);
    }
  RELEASE_STACK (large);
}

static bool
transitive_reduce (kissat * solver,
		   unsigned src, uint64_t limit,
		   uint64_t * reduced_ptr, unsigned *units)
{
  bool res = false;
  assert (!VALUE (src));
  LOG ("transitive reduce %s", LOGLIT (src));
  watches *all_watches = solver->watches;
  watches *src_watches = all_watches + src;
  watch *end_src = END_WATCHES (*src_watches);
  watch *begin_src = BEGIN_WATCHES (*src_watches);
  unsigned ticks = kissat_cache_lines (src_watches->size, sizeof (watch));
  ADD (transitive_ticks, ticks + 1);
  const unsigned not_src = NOT (src);
  unsigned reduced = 0;
  bool failed = false;
  for (watch * p = begin_src; p != end_src; p++)
    {
      const watch src_watch = *p;
      if (!src_watch.type.binary)
	break;
      const unsigned dst = src_watch.binary.lit;
      if (dst < src)
	continue;
      if (VALUE (dst))
	continue;
      assert (solver->propagated == SIZE_STACK (solver->trail));
      unsigned saved = solver->propagated;
      assert (!solver->level);
      solver->level = 1;
      transitive_assign (solver, not_src);
      const bool redundant = src_watch.binary.redundant;
      bool transitive = false;
      unsigned propagated = 0;
      while (!transitive && !failed &&
	     solver->propagated < SIZE_STACK (solver->trail))
	{
	  const unsigned lit = PEEK_STACK (solver->trail, solver->propagated);
	  solver->propagated++;
	  propagated++;
	  LOG ("transitive propagate %s", LOGLIT (lit));
	  assert (VALUE (lit) > 0);
	  const unsigned not_lit = NOT (lit);
	  watches *lit_watches = all_watches + not_lit;
	  const watch *end_lit = END_WATCHES (*lit_watches);
	  const watch *begin_lit = BEGIN_WATCHES (*lit_watches);
	  ticks = kissat_cache_lines (lit_watches->size, sizeof (watch));
	  ADD (transitive_ticks, ticks + 1);
	  for (const watch * q = begin_lit; q != end_lit; q++)
	    {
	      if (p == q)
		continue;
	      const watch lit_watch = *q;
	      if (!lit_watch.type.binary)
		break;
	      if (not_lit == src && lit_watch.binary.lit == ILLEGAL_LIT)
		continue;
	      if (!redundant && lit_watch.binary.redundant)
		continue;
	      const unsigned other = lit_watch.binary.lit;
	      if (other == dst)
		{
		  transitive = true;
		  break;
		}
	      const value value = VALUE (other);
	      if (value < 0)
		{
		  LOG ("both %s and %s reachable from %s",
		       LOGLIT (NOT (other)), LOGLIT (other), LOGLIT (src));
		  failed = true;
		  break;
		}
	      if (!value)
		transitive_assign (solver, other);
	    }
	}

      assert (solver->probing);
      ADD (propagations, propagated);
      ADD (probing_propagations, propagated);
      ADD (transitive_propagations, propagated);

      transitive_backtrack (solver, saved);

      if (transitive)
	{
	  LOGBINARY (src, dst, "transitive reduce");
	  INC (transitive_reduced);
	  watches *dst_watches = all_watches + dst;
	  watch dst_watch = src_watch;
	  assert (dst_watch.binary.lit == dst);
	  assert (dst_watch.binary.redundant == redundant);
	  dst_watch.binary.lit = src;
	  REMOVE_WATCHES (*dst_watches, dst_watch);
	  kissat_delete_binary (solver,
				src_watch.binary.redundant,
				src_watch.binary.hyper, src, dst);
	  p->binary.lit = ILLEGAL_LIT;
	  reduced++;
	  res = true;
	}

      if (failed)
	break;
      if (solver->statistics.transitive_ticks > limit)
	break;
      if (TERMINATED (16))
	break;
    }

  if (reduced)
    {
      *reduced_ptr += reduced;
      assert (begin_src == BEGIN_WATCHES (WATCHES (src)));
      assert (end_src == END_WATCHES (WATCHES (src)));
      watch *q = begin_src;
      for (const watch * p = begin_src; p != end_src; p++)
	{
	  const watch src_watch = *q++ = *p;
	  if (!src_watch.type.binary)
	    {
	      *q++ = *++p;
	      continue;
	    }
	  if (src_watch.binary.lit == ILLEGAL_LIT)
	    q--;
	}
      assert (end_src - q == (ptrdiff_t) reduced);
      SET_END_OF_WATCHES (*src_watches, q);
    }

  if (failed)
    {
      LOG ("transitive failed literal %s", LOGLIT (not_src));
      INC (failed);
      *units += 1;
      res = true;

      kissat_assign_unit (solver, src);
      CHECK_AND_ADD_UNIT (src);
      ADD_UNIT_TO_PROOF (src);

      clause *conflict = kissat_probing_propagate (solver, 0);
      if (conflict)
	{
	  (void) kissat_analyze (solver, conflict);
	  assert (solver->inconsistent);
	}
      else
	{
	  assert (solver->unflushed);
	  kissat_flush_trail (solver);
	}
    }

  return res;
}

void
kissat_transitive_reduction (kissat * solver)
{
  if (solver->inconsistent)
    return;
  assert (solver->watching);
  assert (solver->probing);
  assert (!solver->level);
  if (!GET_OPTION (transitive))
    return;
  if (TERMINATED (17))
    return;
  START (transitive);
  prioritize_binaries (solver);
  bool success = false;
  uint64_t reduced = 0;
  unsigned units = 0;

  SET_EFFICIENCY_BOUND (limit, transitive, transitive_ticks, search_ticks, 0);

  assert (solver->transitive < LITS);
  const unsigned end = solver->transitive;
#ifndef QUIET
  const unsigned active = solver->active;
#endif
  unsigned probed = 0;
  do
    {
      const unsigned lit = solver->transitive++;
      if (solver->transitive == LITS)
	solver->transitive = 0;
      if (!ACTIVE (IDX (lit)))
	continue;
      probed++;
      if (transitive_reduce (solver, lit, limit, &reduced, &units))
	success = true;
      if (solver->inconsistent)
	break;
      if (solver->statistics.transitive_ticks > limit)
	break;
      if (TERMINATED (18))
	break;
    }
  while (solver->transitive != end);
  kissat_phase (solver, "transitive", GET (probings),
		"probed %u (%.0f%%): reduced %" PRIu64 ", units %u",
		probed, kissat_percent (probed, 2 * active), reduced, units);
  STOP (transitive);
  REPORT (!success, 't');
#ifdef QUIET
  (void) success;
#endif
}
