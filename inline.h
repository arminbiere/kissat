#ifndef _inline_h_INCLUDED
#define _inline_h_INCLUDED

#include "internal.h"
#include "logging.h"

#ifndef NMETRICS

static inline size_t
kissat_allocated (kissat * solver)
{
  return solver->statistics.allocated_current;
}

#endif

static inline bool
kissat_propagated (kissat * solver)
{
  return SIZE_STACK (solver->trail) == solver->propagated;
}

static inline value
kissat_fixed (kissat * solver, unsigned lit)
{
  assert (lit < LITS);
  const value res = solver->values[lit];
  if (!res)
    return 0;
  if (LEVEL (lit))
    return 0;
  return res;
}

static inline void
kissat_mark_removed_literal (kissat * solver, unsigned lit)
{
  const unsigned idx = IDX (lit);
  flags *flags = FLAGS (idx);
  if (flags->eliminate)
    return;
  LOG ("marking variable %u removed", idx);
  flags->eliminate = true;
  INC (variables_removed);
}

static inline void
kissat_mark_added_literal (kissat * solver, unsigned lit)
{
  const unsigned idx = IDX (lit);
  flags *flags = FLAGS (idx);
  if (flags->subsume)
    return;
  LOG ("marking variable %u added", idx);
  flags->subsume = true;
  INC (variables_added);
}

static inline void
kissat_push_large_watch (kissat * solver, watches * watches, reference ref)
{
  const watch watch = kissat_large_watch (ref);
  PUSH_WATCHES (*watches, watch);
}

static inline void
kissat_push_binary_watch (kissat * solver, watches * watches,
			  bool redundant, bool hyper, unsigned other)
{
  const watch watch = kissat_binary_watch (other, redundant, hyper);
  PUSH_WATCHES (*watches, watch);
}

static inline void
kissat_push_blocking_watch (kissat * solver, watches * watches,
			    unsigned blocking, reference ref)
{
  assert (solver->watching);
  const watch head = kissat_blocking_watch (blocking);
  PUSH_WATCHES (*watches, head);
  const watch tail = kissat_large_watch (ref);
  PUSH_WATCHES (*watches, tail);
}

static inline void
kissat_watch_other (kissat * solver,
		    bool redundant, bool hyper, unsigned lit, unsigned other)
{
  LOGBINARY (lit, other,
	     "watching %s blocking %s in %s",
	     LOGLIT (lit), LOGLIT (other),
	     (redundant ? "redundant" : "irredundant"));
  watches *watches = &WATCHES (lit);
  kissat_push_binary_watch (solver, watches, redundant, hyper, other);
}

static inline void
kissat_watch_binary (kissat * solver,
		     bool redundant, bool hyper, unsigned a, unsigned b)
{
  kissat_watch_other (solver, redundant, hyper, a, b);
  kissat_watch_other (solver, redundant, hyper, b, a);
}

static inline void
kissat_watch_blocking (kissat * solver,
		       unsigned lit, unsigned blocking, reference ref)
{
  assert (solver->watching);
  LOGREF (ref, "watching %s blocking %s in", LOGLIT (lit), LOGLIT (blocking));
  watches *watches = &WATCHES (lit);
  kissat_push_blocking_watch (solver, watches, blocking, ref);
}

static inline void
kissat_unwatch_blocking (kissat * solver, unsigned lit, reference ref)
{
  assert (solver->watching);
  LOGREF (ref, "unwatching %s in", LOGLIT (lit));
  watches *watches = &WATCHES (lit);
  kissat_remove_blocking_watch (solver, watches, ref);
}

static inline void
kissat_disconnect_binary (kissat * solver, unsigned lit, unsigned other)
{
  assert (!solver->watching);
  watches *watches = &WATCHES (lit);
  const watch watch = kissat_binary_watch (other, false, false);
  REMOVE_WATCHES (*watches, watch);
}

static inline void
kissat_disconnect_reference (kissat * solver, unsigned lit, reference ref)
{
  assert (!solver->watching);
  LOGREF (ref, "disconnecting %s in", LOGLIT (lit));
  const watch watch = kissat_large_watch (ref);
  watches *watches = &WATCHES (lit);
  REMOVE_WATCHES (*watches, watch);
}

static inline void
kissat_watch_reference (kissat * solver,
			unsigned a, unsigned b, reference ref)
{
  assert (solver->watching);
  kissat_watch_blocking (solver, a, b, ref);
  kissat_watch_blocking (solver, b, a, ref);
}

static inline void
kissat_connect_literal (kissat * solver, unsigned lit, reference ref)
{
  assert (!solver->watching);
  LOGREF (ref, "connecting %s in", LOGLIT (lit));
  watches *watches = &WATCHES (lit);
  kissat_push_large_watch (solver, watches, ref);
}

static inline clause *
kissat_unchecked_dereference_clause (kissat * solver, reference ref)
{
  return (clause *) & PEEK_STACK (solver->arena, ref);
}

static inline clause *
kissat_dereference_clause (kissat * solver, reference ref)
{
  clause *res = kissat_unchecked_dereference_clause (solver, ref);
  assert (kissat_clause_in_arena (solver, res));
  return res;
}

static inline reference
kissat_reference_clause (kissat * solver, const clause * c)
{
  assert (kissat_clause_in_arena (solver, c));
  return (word *) c - BEGIN_STACK (solver->arena);
}

static inline void
kissat_inlined_connect_clause (kissat * solver, watches * all_watches,
			       clause * c, reference ref)
{
  assert (!solver->watching);
  assert (ref == kissat_reference_clause (solver, c));
  assert (c == kissat_dereference_clause (solver, ref));
  for (all_literals_in_clause (lit, c))
    {
      assert (!solver->watching);
      LOGREF (ref, "connecting %s in", LOGLIT (lit));
      assert (lit < LITS);
      watches *lit_watches = all_watches + lit;
      kissat_push_large_watch (solver, lit_watches, ref);
    }
}

static inline void
kissat_watch_clause (kissat * solver, clause * c)
{
  assert (c->searched < c->size);
  const reference ref = kissat_reference_clause (solver, c);
  kissat_watch_reference (solver, c->lits[0], c->lits[1], ref);
}

static inline void
kissat_update_queue (kissat * solver, const links * links, unsigned idx)
{
  assert (!DISCONNECTED (idx));
  const unsigned stamp = links[idx].stamp;
  LOG ("queue updated to variable %u stamped %u", idx, stamp);
  solver->queue.search.idx = idx;
  solver->queue.search.stamp = stamp;
}

static inline int
kissat_export_literal (kissat * solver, unsigned ilit)
{
  const unsigned iidx = IDX (ilit);
  assert (iidx < (unsigned) INT_MAX);
  int elit = PEEK_STACK (solver->export, iidx);
  if (!elit)
    return 0;
  if (NEGATED (ilit))
    elit = -elit;
  assert (VALID_EXTERNAL_LITERAL (elit));
  return elit;
}

static inline unsigned
kissat_map_literal (kissat * solver, unsigned ilit, bool map)
{
  if (!map)
    return ilit;
  int elit = kissat_export_literal (solver, ilit);
  if (!elit)
    return INVALID_LIT;
  const unsigned eidx = ABS (elit);
  const import *import = &PEEK_STACK (solver->import, eidx);
  if (import->eliminated)
    return INVALID_LIT;
  unsigned mlit = import->lit;
  if (elit < 0)
    mlit = NOT (mlit);
  return mlit;
}

static inline void
kissat_update_variable_score (kissat * solver, heap * schedule, unsigned idx)
{
  assert (schedule->size);
  const unsigned lit = LIT (idx);
  const unsigned not_lit = NOT (lit);
  size_t pos = WATCHES (lit).size;
  size_t neg = WATCHES (not_lit).size;
  double new_score = ((double) pos) * neg + pos + neg;
  LOG ("new elimination score %g for variable %u (pos %zu and neg %zu)",
       new_score, idx, pos, neg);
  kissat_update_heap (solver, schedule, idx, -new_score);
}

static inline clause *
kissat_last_irredundant_clause (kissat * solver)
{
  return (solver->last_irredundant == INVALID_REF) ? 0 :
    kissat_dereference_clause (solver, solver->last_irredundant);
}

static inline clause *
kissat_binary_conflict (kissat * solver,
			bool redundant, unsigned a, unsigned b)
{
  LOGBINARY (a, b, "conflicting");
  clause *res = &solver->conflict;
  res->redundant = redundant;
  res->size = 2;
  unsigned *lits = res->lits;
  lits[0] = a;
  lits[1] = b;
  return res;
}

static inline int
kissat_checking (kissat * solver)
{
#ifndef NDEBUG
#ifdef NOPTIONS
  (void) solver;
#endif
  return GET_OPTION (check);
#else
  (void) solver;
  return 0;
#endif
}

static inline bool
kissat_logging (kissat * solver)
{
#ifdef LOGGING
#ifdef NOPTIONS
  (void) solver;
#endif
  return GET_OPTION (log) > 0;
#else
  (void) solver;
  return false;
#endif
}

static inline bool
kissat_proving (kissat * solver)
{
#ifdef NPROOFS
  (void) solver;
  return false;
#else
  return solver->proof != 0;
#endif
}

static inline bool
kissat_checking_or_proving (kissat * solver)
{
  return kissat_checking (solver) || kissat_proving (solver);
}

#if !defined(NDEBUG) || !defined(NPROOFS)
#define CHECKING_OR_PROVING
#endif

#endif
