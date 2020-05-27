#include "decide.h"
#include "inline.h"

#include <inttypes.h>

static unsigned
last_enqueued_unassigned_variable (kissat * solver)
{
  assert (solver->unassigned);
  const links *links = solver->links;
  const value *values = solver->values;
  unsigned res = solver->queue.search.idx;
  if (values[LIT (res)])
    {
      do
	{
	  res = links[res].prev;
	  assert (!DISCONNECTED (res));
	}
      while (values[LIT (res)]);
      kissat_update_queue (solver, links, res);
    }
#ifdef LOGGING
  const unsigned stamp = links[res].stamp;
  LOG ("last enqueued unassigned variable %u stamp %u", res, stamp);
#endif
#ifdef CHECK_QUEUE
  for (unsigned i = links[res].next; !DISCONNECTED (i); i = links[i].next)
    assert (VALUE (LIT (i)));
#endif
  return res;
}

static unsigned
largest_score_unassigned_variable (kissat * solver)
{
  unsigned res = kissat_max_heap (&solver->scores);
  const value *values = solver->values;
  while (values[LIT (res)])
    {
      kissat_pop_heap (solver, &solver->scores, res);
      res = kissat_max_heap (&solver->scores);
    }
#if defined(LOGGING) || defined(CHECK_HEAP)
  const double score = kissat_get_heap_score (&solver->scores, res);
#endif
  LOG ("largest score unassigned variable %u score %g", res, score);
#ifdef CHECK_HEAP
  for (all_variables (idx))
    {
      if (VALUE (LIT (idx)))
	continue;
      const double idx_score = kissat_get_heap_score (&solver->scores, idx);
      assert (score >= idx_score);
    }
#endif
  return res;
}

unsigned
kissat_next_decision_variable (kissat * solver)
{
  unsigned res;
  if (solver->stable)
    res = largest_score_unassigned_variable (solver);
  else
    res = last_enqueued_unassigned_variable (solver);
  LOG ("next decision variable %u", res);
  return res;
}

static inline value
decide_phase (kissat * solver, unsigned idx)
{
  bool target;
  if (!GET_OPTION (target))
    target = false;
  else if (solver->stable)
    target = true;
  else
    target = (GET_OPTION (target) > 1);

  const phase *phase = PHASE (idx);
  value res = 0;

  if (target && (res = phase->target))
    {
      LOG ("variable %u uses target decision phase %d", idx, (int) res);
      INC (target_decisions);
    }

  if (!res && (res = phase->saved))
    {
      LOG ("variable %u uses saved decision phase %d", idx, (int) res);
      INC (saved_decisions);
    }

  if (!res)
    {
      res = INITIAL_PHASE;
      LOG ("variable %u uses initial decision phase %d", idx, (int) res);
      INC (initial_decisions);
    }
  assert (res);

  return res;
}

void
kissat_decide (kissat * solver)
{
  START (decide);
  assert (solver->unassigned);
  INC (decisions);
  assert (solver->level < MAX_LEVEL);
  solver->level++;
  const unsigned idx = kissat_next_decision_variable (solver);
  const value value = decide_phase (solver, idx);
  unsigned lit = LIT (idx);
  if (value < 0)
    lit = NOT (lit);
  kissat_push_frame (solver, lit);
  assert (solver->level < SIZE_STACK (solver->frames));
  LOG ("decide literal %s", LOGLIT (lit));
  kissat_assign_decision (solver, lit);
  STOP (decide);
}

void
kissat_internal_assume (kissat * solver, unsigned lit)
{
  assert (solver->unassigned);
  assert (!VALUE (lit));
  assert (solver->level < MAX_LEVEL);
  solver->level++;
  kissat_push_frame (solver, lit);
  assert (solver->level < SIZE_STACK (solver->frames));
  LOG ("assuming literal %s", LOGLIT (lit));
  kissat_assign_decision (solver, lit);
}
