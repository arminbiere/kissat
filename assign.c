#include "inline.h"
#include "assign.h"
#include "logging.h"

#include <limits.h>

static inline void
kissat_assign (kissat * solver,
#ifdef INLINE_ASSIGN
	       value * values, assigned * assigned,
#endif
	       unsigned lit,
	       bool binary, bool redundant, unsigned level, unsigned reason)
{
  assert (binary || !redundant);
  const unsigned not_lit = NOT (lit);
#ifndef INLINE_ASSIGN
  value *values = solver->values;
  assigned *assigned = solver->assigned;
#endif
  assert (!values[lit]);
  assert (!values[not_lit]);

  values[lit] = 1;
  values[not_lit] = -1;

  assert (solver->unassigned > 0);
  solver->unassigned--;

  const unsigned idx = IDX (lit);
  struct assigned *a = assigned + idx;

  if (level)
    {
      a->level = level;
      a->binary = binary;
      a->redundant = redundant;
      a->reason = reason;
    }
  else
    {
      a->level = 0;
      a->binary = false;
      a->redundant = false;
      a->reason = UNIT;
    }

  if (!solver->probing)
    {
      const bool negated = NEGATED (lit);
      const value value = BOOL_TO_VALUE (negated);
      SAVED (idx) = value;
    }

  PUSH_STACK (solver->trail, lit);

  if (!level)
    {
      kissat_mark_fixed_literal (solver, lit);
      assert (solver->unflushed < UINT_MAX);
      solver->unflushed++;
    }

  watches *watches = &WATCHES (not_lit);
  if (!watches->size)
    {
      watch *w = BEGIN_WATCHES (*watches);
      __builtin_prefetch (w, 0, 1);
    }
}

static inline unsigned
kissat_assignment_level (kissat * solver,
			 value * values, assigned * assigned,
			 unsigned lit, clause * reason)
{
  unsigned res = 0;
  for (all_literals_in_clause (other, reason))
    {
      if (other == lit)
	continue;
      assert (values[other] < 0), (void) values;
      const unsigned other_idx = IDX (other);
      struct assigned *a = assigned + other_idx;
      const unsigned level = a->level;
      if (res < level)
	res = level;
    }
#ifdef NDEBUG
  (void) solver;
#endif
  return res;
}

#ifndef INLINE_ASSIGN

void
kissat_assign_unit (kissat * solver, unsigned lit)
{
  kissat_assign (solver, lit, false, false, 0, UNIT);
  LOGUNARY (lit, "assign %s reason", LOGLIT (lit));
}

void
kissat_assign_decision (kissat * solver, unsigned lit)
{
  kissat_assign (solver, lit, false, false, solver->level, DECISION);
  LOG ("assign %s decision", LOGLIT (lit));
}

#endif

#ifdef INLINE_ASSIGN
static inline
#endif
  void
kissat_assign_binary (kissat * solver,
#ifdef INLINE_ASSIGN
		      value * values, assigned * assigned,
#endif
		      bool redundant, unsigned lit, unsigned other)
{
  assert (VALUE (other) < 0);
#ifndef INLINE_ASSIGN
  assigned *assigned = solver->assigned;
#endif
  const unsigned other_idx = IDX (other);
  struct assigned *a = assigned + other_idx;
  const unsigned level = a->level;
  kissat_assign (solver,
#ifdef INLINE_ASSIGN
		 values, assigned,
#endif
		 lit, true, redundant, level, other);
  LOGBINARY (lit, other, "assign %s %s reason",
	     LOGLIT (lit), redundant ? "redundant" : "irredundant");

}

#ifdef INLINE_ASSIGN
static inline
#endif
  void
kissat_assign_reference (kissat * solver,
#ifdef INLINE_ASSIGN
			 value * values, assigned * assigned,
#endif
			 unsigned lit, reference ref, clause * reason)
{
  assert (reason == kissat_dereference_clause (solver, ref));
#ifndef INLINE_ASSIGN
  assigned *assigned = solver->assigned;
  value *values = solver->values;
#endif
  const unsigned level =
    kissat_assignment_level (solver, values, assigned, lit, reason);
  assert (ref != DECISION);
  assert (ref != UNIT);
  kissat_assign (solver,
#ifdef INLINE_ASSIGN
		 values, assigned,
#endif
		 lit, false, false, level, ref);
  LOGREF (ref, "assign %s reason", LOGLIT (lit));
}
