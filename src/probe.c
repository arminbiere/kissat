#include "probe.h"
#include "backbone.h"
#include "backtrack.h"
#include "internal.h"
#include "print.h"
#include "substitute.h"
#include "sweep.h"
#include "transitive.h"
#include "vivify.h"

#include <inttypes.h>

bool kissat_probing (kissat *solver) {
  if (!solver->enabled.probe)
    return false;
  if (solver->waiting.probe.reduce > solver->statistics.reductions)
    return false;
  return solver->limits.probe.conflicts <= CONFLICTS;
}

static void probe (kissat *solver) {
  kissat_backtrack_propagate_and_flush_trail (solver);
  assert (!solver->inconsistent);
  STOP_SEARCH_AND_START_SIMPLIFIER (probe);
  kissat_phase (solver, "probe", GET (probings),
                "probing limit hit after %" PRIu64 " conflicts",
                solver->limits.probe.conflicts);
  kissat_substitute (solver);
  kissat_binary_clauses_backbone (solver);
  kissat_vivify (solver);
  kissat_sweep (solver);
  kissat_substitute (solver);
  kissat_transitive_reduction (solver);
  kissat_binary_clauses_backbone (solver);
  STOP_SIMPLIFIER_AND_RESUME_SEARCH (probe);
}

int kissat_probe (kissat *solver) {
  assert (!solver->inconsistent);
  INC (probings);
  assert (!solver->probing);
  solver->probing = true;
  probe (solver);
  UPDATE_CONFLICT_LIMIT (probe, probings, NLOGN, true);
  solver->waiting.probe.reduce = solver->statistics.reductions + 1;
  solver->last.probe = solver->statistics.search_ticks;
  assert (solver->probing);
  solver->probing = false;
  return solver->inconsistent ? 20 : 0;
}
