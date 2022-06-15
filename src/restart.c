#include "backtrack.h"
#include "branching.h"
#include "bump.h"
#include "decide.h"
#include "internal.h"
#include "logging.h"
#include "kimits.h"
#include "print.h"
#include "reluctant.h"
#include "report.h"
#include "restart.h"

#include <inttypes.h>

bool
kissat_restarting (kissat * solver)
{
  assert (solver->unassigned);
  if (!GET_OPTION (restart))
    return false;
  if (!solver->level)
    return false;
  if (CONFLICTS < solver->limits.restart.conflicts)
    return false;
  if (solver->stable)
    return kissat_reluctant_triggered (&solver->reluctant);
  const double fast = AVERAGE (fast_glue);
  const double slow = AVERAGE (slow_glue);
  const double margin = (100.0 + GET_OPTION (restartmargin)) / 100.0;
  const double limit = margin * slow;
  LOG ("restart glue limit %g = %.02f * %g (slow glue) %c %g (fast glue)",
       limit, margin, slow,
       (limit > fast ? '>' : limit == fast ? '=' : '<'), fast);
  return (limit <= fast);
}

void
kissat_new_focused_restart_limit (kissat * solver)
{
  assert (!solver->stable);
  limits *limits = &solver->limits;
  uint64_t restarts = solver->statistics.restarts;
  uint64_t delta = GET_OPTION (restartint);
  if (restarts)
    delta += kissat_logn (restarts) - 1;
  limits->restart.conflicts = CONFLICTS + delta;
  kissat_extremely_verbose (solver,
			    "focused restart limit at %"
			    PRIu64 " after %" PRIu64 " conflicts ",
			    limits->restart.conflicts, delta);
}

static unsigned
reuse_stable_trail (kissat * solver)
{
  const unsigned next_idx = kissat_next_decision_variable (solver);
  const struct assigned *assigned = solver->assigned;
  const heap *const scores = SCORES;
  const unsigned next_idx_score = kissat_get_heap_score (scores, next_idx);
  LOG ("next decision variable score %u", next_idx_score);
  double decision_score = MAX_SCORE;
  for (all_stack (unsigned, lit, solver->trail))
    {
      const unsigned idx = IDX (lit);
      const double score = kissat_get_heap_score (scores, idx);
      const struct assigned *const a = assigned + idx;
      if (decision_score < score || score < next_idx_score)
	{
	  const unsigned level = a->level;
	  return level ? level - 1 : 0;
	}
      if (a->reason == DECISION_REASON)
	decision_score = score;
    }
  return solver->level;
}

static unsigned
reuse_focused_trail (kissat * solver)
{
  const unsigned next_idx = kissat_next_decision_variable (solver);
  const struct assigned *assigned = solver->assigned;
  const links *const links = solver->links;
  const unsigned next_idx_stamp = links[next_idx].stamp;
  LOG ("next decision variable stamp %u", next_idx_stamp);
  unsigned decision_stamp = UINT_MAX;
  for (all_stack (unsigned, lit, solver->trail))
    {
      const unsigned idx = IDX (lit);
      const unsigned stamp = links[idx].stamp;
      const struct assigned *const a = assigned + idx;
      if (decision_stamp < stamp || stamp < next_idx_stamp)
	{
	  const unsigned level = a->level;
	  return level ? level - 1 : 0;
	}
      if (a->reason == DECISION_REASON)
	decision_stamp = stamp;
    }
  return solver->level;
}

static unsigned
smallest_out_of_order_trail_level (kissat * solver)
{
  unsigned max_level = 0;
  unsigned res = INVALID_LEVEL;
  const struct assigned *const assigned = solver->assigned;
  for (all_stack (unsigned, lit, solver->trail))
    {
      const unsigned idx = IDX (lit);
      const struct assigned *const a = assigned + idx;
      const unsigned level = a->level;
      if (level < max_level && level < res && !(res = level))
	break;
      if (level > max_level)
	max_level = level;
    }
#ifdef LOGGING
  if (res < INVALID_LEVEL)
    LOG ("out-of-order trail with smallest out-of-order level %u", res);
  else
    LOG ("trail respects decision order");
#endif
  return res;
}

static unsigned
reuse_trail (kissat * solver)
{
  assert (solver->level);
  assert (!EMPTY_STACK (solver->trail));

  const int option = GET_OPTION (reusetrail);
  if (!option)
    return 0;
  if (option < 2 && solver->stable)
    return 0;

  unsigned res;

  if (solver->stable)
    res = reuse_stable_trail (solver);
  else
    res = reuse_focused_trail (solver);

  LOG ("matching trail level %u", res);

  if (res)
    {
      unsigned smallest = smallest_out_of_order_trail_level (solver);
      if (smallest < res)
	res = smallest;
    }

  if (res)
    {
      INC (restarts_reused_trails);
      ADD (reused_levels, res);
      LOG ("restart reuses trail at decision level %u", res);
    }
  else
    LOG ("restarts does not reuse the trail");

  return res;
}

void
kissat_restart (kissat * solver)
{
  START (restart);
  INC (restarts);
  if (solver->stable)
    INC (stable_restarts);
  else
    INC (focused_restarts);
  unsigned level = reuse_trail (solver);
  kissat_extremely_verbose (solver,
			    "restarting after %" PRIu64 " conflicts"
			    " (limit %" PRIu64 ")", CONFLICTS,
			    solver->limits.restart.conflicts);
  LOG ("restarting to level %u", level);
  kissat_backtrack_in_consistent_state (solver, level);
  if (!solver->stable)
    kissat_new_focused_restart_limit (solver);
  else if (kissat_toggle_branching (solver))
    kissat_update_scores (solver);
  REPORT (1, 'R');
  STOP (restart);
}
