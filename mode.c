#include "inline.h"
#include "print.h"
#include "reluctant.h"
#include "rephase.h"
#include "report.h"
#include "restart.h"
#include "resources.h"

#include <inttypes.h>

static void
new_mode_limit (kissat * solver)
{
  kissat_init_averages (solver, &AVERAGES);

  limits *limits = &solver->limits;
  statistics *statistics = &solver->statistics;

  assert (GET_OPTION (stable) == 1);

  if (solver->stable)
    {
      assert (solver->mode.ticks <= statistics->search_ticks);
      uint64_t ticks = statistics->search_ticks - solver->mode.ticks;

      const double bias = GET_OPTION (stablebias) * 1e-2;
      ticks *= bias;

      limits->mode.ticks = statistics->search_ticks + ticks;
      kissat_phase (solver, "stable", GET (stable_modes),
		    "new focused mode switching limit of %s "
		    "after %s search ticks bias %.0f%%",
		    FORMAT_COUNT (limits->mode.ticks),
		    FORMAT_COUNT (ticks), 100 * bias);
    }
  else
    {
      const uint64_t interval = GET_OPTION (modeint);
      const uint64_t count = statistics->stable_modes;
      const uint64_t delta = interval * (kissat_quadratic (count) + 1);
      limits->mode.conflicts = CONFLICTS + delta;
      kissat_phase (solver, "focus", GET (focused_modes),
		    "new stable mode switching limit of %s "
		    "after %s conflicts",
		    FORMAT_COUNT (limits->mode.conflicts),
		    FORMAT_COUNT (delta));
    }


  kissat_reset_rephased (solver);
#ifndef QUIET
  solver->mode.conflicts = statistics->conflicts;
#ifndef NMETRICS
  solver->mode.propagations = statistics->search_propagations;
#endif
#endif
  solver->mode.ticks = statistics->search_ticks;
}

static void
report_switching_from_mode (kissat * solver)
{
#ifndef QUIET
  if (kissat_verbosity (solver) < 2)
    return;

  const double current_time = kissat_process_time ();
  const double delta_time = current_time - solver->mode.entered;

  statistics *statistics = &solver->statistics;

  const uint64_t delta_conflicts =
    statistics->conflicts - solver->mode.conflicts;

  const uint64_t delta_ticks = statistics->search_ticks - solver->mode.ticks;
#ifndef NMETRICS
  const uint64_t delta_propagations =
    statistics->search_propagations - solver->mode.propagations;
#endif
  solver->mode.entered = current_time;

  // *INDENT-OFF*
  kissat_very_verbose (solver, "%s mode took %.2f seconds "
    "(%" PRIu64 " conflicts, %" PRIu64 " ticks"
#ifndef NMETRICS
    ", %" PRIu64 " propagations"
#endif
    ")", solver->stable ? "stable" : "focused",
    delta_time, delta_conflicts, delta_ticks
#ifndef NMETRICS
    , delta_propagations
#endif
    );
  // *INDENT-ON*
#else
  (void) solver;
#endif
}

static void
switch_to_focused_mode (kissat * solver)
{
  assert (solver->stable);
  report_switching_from_mode (solver);
  REPORT (0, ']');
  STOP (stable);
  INC (focused_modes);
  kissat_phase (solver, "focus", GET (focused_modes),
		"switching to focused mode after %" PRIu64
		" conflicts", CONFLICTS);
  solver->stable = false;
  new_mode_limit (solver);
  START (focused);
  REPORT (0, '{');
  const unsigned last = solver->queue.last;
  assert (!DISCONNECTED (last));
  kissat_update_queue (solver, solver->links, last);
  kissat_new_focused_restart_limit (solver);
}

void
kissat_update_scores (kissat * solver)
{
  heap *scores = &solver->scores;
  for (all_variables (idx))
    if (ACTIVE (idx) && !kissat_heap_contains (scores, idx))
      kissat_push_heap (solver, scores, idx);
}

static void
switch_to_stable_mode (kissat * solver)
{
  assert (!solver->stable);
  report_switching_from_mode (solver);
  REPORT (0, '}');
  STOP (focused);
  INC (stable_modes);
  solver->stable = true;
  kissat_phase (solver, "stable", GET (stable_modes),
		"switched to stable mode after %" PRIu64
		" conflicts", CONFLICTS);
  new_mode_limit (solver);
  START (stable);
  REPORT (0, '[');
  kissat_init_reluctant (solver);
  kissat_update_scores (solver);
}

void
kissat_switch_search_mode (kissat * solver)
{
  assert (!solver->inconsistent);
  if (solver->rephased.type)
    return;

  if (GET_OPTION (stable) != 1)
    return;

  limits *limits = &solver->limits;
  statistics *statistics = &solver->statistics;

  if (solver->stable)
    {
      if (statistics->search_ticks < limits->mode.ticks)
	return;
    }
  else if (statistics->conflicts < limits->mode.conflicts)
    return;

  INC (switched_modes);

  if (solver->stable)
    switch_to_focused_mode (solver);
  else
    switch_to_stable_mode (solver);
}
