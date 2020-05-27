#include "backtrack.h"
#include "failed.h"
#include "internal.h"
#include "print.h"
#include "probe.h"
#include "ternary.h"
#include "transitive.h"
#include "substitute.h"
#include "vivify.h"

#include <inttypes.h>

bool
kissat_probing (kissat * solver)
{
  if (!solver->enabled.probe)
    return false;
  if (solver->waiting.probe.reduce > solver->statistics.reductions)
    return false;
  return solver->limits.probe.conflicts <= CONFLICTS;
}

static void
probe (kissat * solver)
{
  RETURN_IF_DELAYED (probe);
  kissat_backtrack_propagate_and_flush_trail (solver);
  assert (!solver->inconsistent);
  STOP_SEARCH_AND_START_SIMPLIFIER (probe);
  kissat_phase (solver, "probe", GET (probings),
		"probing limit hit after %" PRIu64 " conflicts",
		solver->limits.probe.conflicts);
  assert (!solver->probing);
  solver->probing = true;
  const changes before = kissat_changes (solver);
  kissat_substitute (solver, true);
  kissat_ternary (solver);
  kissat_transitive_reduction (solver);
  kissat_failed_literal_probing (solver);
  kissat_vivify (solver);
  kissat_substitute (solver, false);
  assert (solver->probing);
  solver->probing = false;
  const changes after = kissat_changes (solver);
  const bool changed = kissat_changed (before, after);
  UPDATE_DELAY (changed, probe);
  STOP_SIMPLIFIER_AND_RESUME_SEARCH (probe);
}

int
kissat_probe (kissat * solver)
{
  assert (!solver->inconsistent);
  INC (probings);
  probe (solver);
  UPDATE_CONFLICT_LIMIT (probe, probings, NLOGN, true);
  solver->waiting.probe.reduce = solver->statistics.reductions + 1;
  return solver->inconsistent ? 20 : 0;
}
