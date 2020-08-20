#include "internal.h"
#include "logging.h"
#include "print.h"
#include "reduce.h"
#include "resources.h"
#include "restart.h"

#include <inttypes.h>
#include <math.h>

changes
kissat_changes (kissat * solver)
{
  changes res;
  res.variables.units = solver->statistics.units;
  res.variables.added = solver->statistics.variables_added;
  res.variables.removed = solver->statistics.variables_removed;
  res.eliminate.additional_clauses =
    solver->bounds.eliminate.additional_clauses;
  return res;
}

bool
kissat_changed (changes b, changes a)
{
  if (a.variables.added != b.variables.added)
    return true;
  if (a.variables.units != b.variables.units)
    return true;
  if (a.variables.removed != b.variables.removed)
    return true;
  if (a.eliminate.additional_clauses != b.eliminate.additional_clauses)
    return true;
  return false;
}

uint64_t
kissat_logn (uint64_t count)
{
  return log10 (count + 10);
}

static uint64_t
kissat_lognlogn (uint64_t count)
{
  const double tmp = log10 (count + 10);
  return tmp * tmp;
}

uint64_t
kissat_ndivlogn (uint64_t count)
{
  return count / kissat_logn (count);
}

uint64_t
kissat_linear (uint64_t count)
{
  return count;
}

uint64_t
kissat_nlogn (uint64_t count)
{
  return count * kissat_logn (count);
}

uint64_t
kissat_nlognlogn (uint64_t count)
{
  return count * kissat_lognlogn (count);
}

uint64_t
kissat_quadratic (uint64_t count)
{
  return count * count;
}

uint64_t
kissat_scale_delta (kissat * solver, const char *pretty, uint64_t delta)
{
  const uint64_t C = IRREDUNDANT_CLAUSES;
  const double f = kissat_logn (C);
  assert (f >= 1);
  const double ff = f * f;
  assert (ff >= 1);
  uint64_t scaled = ff * delta;
  assert (delta <= scaled);
// *INDENT-OFF*
  kissat_very_verbose (solver,
    "scaled %s delta %" PRIu64
    " = %g * %" PRIu64
    " = %g^2 * %" PRIu64
    " = log10^2(" PRIu64 ") * %" PRIu64,
    pretty, scaled, ff, delta, f, delta, C, delta);
// *INDENT-ON*
  (void) pretty;
  return scaled;
}

uint64_t
kissat_scale_limit (kissat * solver,
		    const char *pretty, uint64_t count, int base)
{
  assert (base >= 0);
  assert (count > 0);
  const double f = kissat_logn (count - 1);
  assert (f >= 1);
  uint64_t scaled = f * base;
  kissat_very_verbose (solver,
		       "scaled %s limit %" PRIu64 " = "
		       "log10 (%" PRIu64 ") * %d = %g * %d",
		       pretty, scaled, count, base, f, base);
  (void) solver;
  (void) pretty;
  return scaled;
}

static void
init_enabled (kissat * solver)
{
  bool probe;
  if (!GET_OPTION (simplify))
    probe = false;
  else if (!GET_OPTION (probe))
    probe = false;
  else if (GET_OPTION (substitute))
    probe = true;
  else if (GET_OPTION (failed))
    probe = true;
  else if (GET_OPTION (transitive))
    probe = true;
  else if (GET_OPTION (vivify))
    probe = true;
  else
    probe = false;
  kissat_very_verbose (solver, "probing %sabled", probe ? "en" : "dis");
  solver->enabled.probe = probe;

  bool eliminate;
  if (!GET_OPTION (simplify))
    eliminate = false;
  else if (!GET_OPTION (eliminate))
    eliminate = false;
  else
    eliminate = true;
  kissat_very_verbose (solver, "eliminate %sabled", eliminate ? "en" : "dis");
  solver->enabled.eliminate = eliminate;

  bool autarky;
  if (!GET_OPTION (simplify))
    autarky = false;
  else if (!GET_OPTION (autarky))
    autarky = false;
  else
    autarky = true;
  kissat_very_verbose (solver, "autarky %sabled", autarky ? "en" : "dis");
  solver->enabled.autarky = autarky;
}

static void
init_mode_limit (kissat * solver)
{
  limits *limits = &solver->limits;

  if (GET_OPTION (stable) == 1)
    {
      assert (!solver->stable);
      const uint64_t delta = GET_OPTION (modeinit);
      const uint64_t limit = CONFLICTS + delta;
      limits->mode.conflicts = limit;
      kissat_very_verbose (solver, "initial stable mode switching limit "
			   "at %s conflicts", FORMAT_COUNT (limit));

      solver->mode.ticks = solver->statistics.search_ticks;
#ifndef QUIET
      solver->mode.conflicts = CONFLICTS;
#ifndef NMETRICS
      solver->mode.propagations = solver->statistics.search_propagations;
#endif
// *INDENT-OFF*
      solver->mode.entered = kissat_process_time ();
      kissat_very_verbose (solver,
        "starting focused mode at %.2f seconds "
        "(%" PRIu64 " conflicts, %" PRIu64 " ticks"
#ifndef NMETRICS
	", %" PRIu64 " propagations, %" PRIu64 " visits"
#endif
	")",
        solver->mode.entered, solver->mode.conflicts, solver->mode.ticks
#ifndef NMETRICS
        , solver->mode.propagations, solver->mode.visits
#endif
	);
// *INDENT-ON*
#endif
    }
  else
    kissat_very_verbose (solver,
			 "no need to set mode limit (only %s mode enabled)",
			 GET_OPTION (stable) ? "stable" : "focused");
}

static void
common_limits (kissat * solver)
{
  init_mode_limit (solver);

  if (solver->enabled.eliminate)
    {
      solver->bounds.eliminate.max_bound_completed = 0;
      solver->bounds.eliminate.additional_clauses = 0;
      kissat_very_verbose (solver, "reset elimination bound to zero");
    }
}

void
kissat_init_limits (kissat * solver)
{
  assert (solver->statistics.searches == 1);

  init_enabled (solver);

  limits *limits = &solver->limits;

  if (GET_OPTION (reduce))
    INIT_CONFLICT_LIMIT (reduce, false);

  if (GET_OPTION (rephase))
    INIT_CONFLICT_LIMIT (rephase, false);

  if (!solver->stable)
    kissat_new_focused_restart_limit (solver);

  common_limits (solver);

  if (solver->enabled.eliminate)
    INIT_CONFLICT_LIMIT (eliminate, true);

  if (solver->enabled.probe)
    INIT_CONFLICT_LIMIT (probe, true);
}
