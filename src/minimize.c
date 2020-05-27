#include "inline.h"
#include "minimize.h"

static bool minimize_literal (kissat *, assigned *, unsigned lit, int depth);

static inline bool
minimize_reference (kissat * solver, assigned * assigned,
		    reference ref, unsigned lit, int depth)
{
  const unsigned not_lit = NOT (lit);
  clause *c = kissat_dereference_clause (solver, ref);
  for (all_literals_in_clause (other, c))
    if (other != not_lit &&
	!minimize_literal (solver, assigned, other, depth))
      return false;
  return true;
}

static inline bool
minimize_binary (kissat * solver, assigned * assigned,
		 unsigned lit, int depth)
{
  const size_t saved = SIZE_STACK (solver->minimize);
  bool res;
  for (unsigned next = lit;;)
    {
      const unsigned next_idx = IDX (next);
      struct assigned *a = assigned + next_idx;
      if (a->analyzed == REMOVABLE)
	{
	  res = true;
	  break;
	}
      if (a->reason == DECISION || a->analyzed == POISONED)
	{
	  res = false;
	  break;
	}
      PUSH_STACK (solver->minimize, next_idx);
      if (!a->binary)
	{
	  res = minimize_reference (solver, assigned,
				    a->reason, next, depth + 1);
	  break;
	}
      next = a->reason;
    }
  unsigned *begin = BEGIN_STACK (solver->minimize) + saved;
  const unsigned *end = END_STACK (solver->minimize);
  assert (begin <= end);
  if (res)
    {
      for (const unsigned *p = begin; p != end; p++)
	{
	  const unsigned other_idx = *p;
	  LOG ("removable variable %u", other_idx);
	  PUSH_STACK (solver->removable, other_idx);
	  struct assigned *a = assigned + other_idx;
	  a->analyzed = REMOVABLE;
	}
    }
  else
    {
      for (const unsigned *p = begin; p != end; p++)
	{
	  const unsigned other_idx = *p;
	  LOG ("poisoned variable %u", other_idx);
	  PUSH_STACK (solver->poisoned, other_idx);
	  struct assigned *a = assigned + other_idx;
	  a->analyzed = POISONED;
	}
    }
  SET_END_OF_STACK (solver->minimize, begin);
  return res;
}

static bool
minimize_literal (kissat * solver, assigned * assigned,
		  unsigned lit, int depth)
{
  assert (VALUE (lit) < 0);
  if (depth >= GET_OPTION (minimizedepth))
    return false;
  const unsigned idx = IDX (lit);
  struct assigned *a = assigned + idx;
  if (!a->level)
    return true;
  if (a->analyzed == REMOVABLE && depth)
    return true;
  assert (a->reason != UNIT);
  if (a->reason == DECISION)
    return false;
  if (a->analyzed == POISONED)
    return false;
  frame *frame = &FRAME (a->level);
  if (frame->used <= 1)
    return false;
  bool res = true;
  if (a->binary)
    res = minimize_binary (solver, assigned, a->reason, depth);
  else
    res = minimize_reference (solver, assigned, a->reason, lit, depth + 1);
  if (!depth)
    assert (a->analyzed == REMOVABLE);
  else if (res)
    {
      LOG ("removable variable %u", idx);
      PUSH_STACK (solver->removable, idx);
      a->analyzed = REMOVABLE;
    }
  else
    {
      LOG ("poisoned variable %u", idx);
      PUSH_STACK (solver->poisoned, idx);
      a->analyzed = POISONED;
    }
  return res;
}

static void
reset_minimize (kissat * solver)
{
  LOG ("unmarking %zu poisoned variables", SIZE_STACK (solver->poisoned));
  for (all_stack (unsigned, idx, solver->poisoned))
      solver->assigned[idx].analyzed = 0;

  LOG ("reset %zu poisoned variables", SIZE_STACK (solver->poisoned));
  CLEAR_STACK (solver->poisoned);

  LOG ("unmarking %zu removable variables", SIZE_STACK (solver->removable));
  for (all_stack (unsigned, idx, solver->removable))
      solver->assigned[idx].analyzed = 0;

  LOG ("reset %zu removable variables", SIZE_STACK (solver->removable));
  CLEAR_STACK (solver->removable);
}

void
kissat_minimize_clause (kissat * solver)
{
  START (minimize);

  assert (EMPTY_STACK (solver->minimize));
  assert (EMPTY_STACK (solver->removable));
  assert (EMPTY_STACK (solver->poisoned));
  assert (!EMPTY_STACK (solver->clause.lits));

  unsigned *lits = BEGIN_STACK (solver->clause.lits);
  const unsigned *end = END_STACK (solver->clause.lits);
  unsigned *q = lits + 1;

  unsigned minimized = 0;

  assigned *assigned = solver->assigned;

  for (const unsigned *p = q; p != end; p++)
    {
      const unsigned lit = *p;
      if (minimize_literal (solver, assigned, lit, 0))
	{
	  LOG ("minimized literal %s", LOGLIT (lit));
	  minimized++;
	}
      else
	{
	  LOG ("keeping literal %s", LOGLIT (lit));
	  *q++ = lit;
	}
    }
  SET_END_OF_STACK (solver->clause.lits, q);
  LOG ("clause minimization removed %u literals", minimized);

  if (!solver->probing)
    ADD (minimized, minimized);

  LOGTMP ("minimized learned");

  reset_minimize (solver);
  STOP (minimize);
}
