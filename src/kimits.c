#include "internal.h"
#include "logging.h"
#include "mode.h"
#include "print.h"
#include "reduce.h"
#include "rephase.h"
#include "resources.h"
#include "restart.h"

#include <inttypes.h>
#include <math.h>

double kissat_logn (uint64_t count) {
  assert (count > 0);
  const double res = log10 (count + 9);
  assert (res >= 1);
  return res;
}

double kissat_sqrt (uint64_t count) {
  assert (count > 0);
  const double res = sqrt (count);
  assert (res >= 1);
  return res;
}

double kissat_nlogpown (uint64_t count, unsigned exponent) {
  assert (count > 0);
  const double tmp = log10 (count + 9);
  double factor = 1;
  while (exponent--)
    factor *= tmp;
  assert (factor >= 1);
  const double res = count * factor;
  assert (res >= 1);
  return res;
}

double kissat_quadratic (uint64_t count) {
  assert (count > 0);
  const double res = count * count;
  assert (res >= 1);
  return res;
}

uint64_t kissat_scale_delta (kissat *solver, const char *pretty,
                             uint64_t delta) {
  const uint64_t C = BINIRR_CLAUSES;
  const double f = kissat_logn (C + 1);
  assert (f >= 1);
  const double ff = f * f;
  assert (ff >= 1);
  uint64_t scaled = ff * delta;
  assert (delta <= scaled);
  // clang-format off
  kissat_very_verbose (solver,
    "scaled %s delta %" PRIu64
    " = %g * %" PRIu64
    " = %g^2 * %" PRIu64
    " = log10^2(%" PRIu64 ") * %" PRIu64,
    pretty, scaled, ff, delta, f, delta, C, delta);
  // clang-format on
  (void) pretty;
  return scaled;
}

static void init_enabled (kissat *solver) {
  bool probe;
  if (!GET_OPTION (simplify))
    probe = false;
  else if (!GET_OPTION (probe))
    probe = false;
  else if (GET_OPTION (substitute))
    probe = true;
  else if (GET_OPTION (sweep))
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
  kissat_very_verbose (solver, "eliminate %sabled",
                       eliminate ? "en" : "dis");
  solver->enabled.eliminate = eliminate;
}

#define INIT_CONFLICT_LIMIT(NAME, SCALE) \
  do { \
    const uint64_t DELTA = GET_OPTION (NAME##init); \
    const uint64_t SCALED = \
        !(SCALE) ? DELTA : kissat_scale_delta (solver, #NAME, DELTA); \
    limits->NAME.conflicts = CONFLICTS + SCALED; \
    kissat_very_verbose (solver, \
                         "initial " #NAME " limit of %s conflicts", \
                         FORMAT_COUNT (limits->NAME.conflicts)); \
  } while (0)

void kissat_init_limits (kissat *solver) {
  assert (solver->statistics.searches == 1);

  init_enabled (solver);

  limits *limits = &solver->limits;

  if (GET_OPTION (randec))
    INIT_CONFLICT_LIMIT (randec, false);

  if (GET_OPTION (reduce))
    INIT_CONFLICT_LIMIT (reduce, false);

  if (GET_OPTION (rephase))
    INIT_CONFLICT_LIMIT (rephase, false);

  if (!solver->stable)
    kissat_update_focused_restart_limit (solver);

  kissat_init_mode_limit (solver);

  if (solver->enabled.eliminate) {
    INIT_CONFLICT_LIMIT (eliminate, true);
    solver->bounds.eliminate.max_bound_completed = 0;
    solver->bounds.eliminate.additional_clauses = 0;
    kissat_very_verbose (solver, "reset elimination bound to zero");
  }

  if (solver->enabled.probe)
    INIT_CONFLICT_LIMIT (probe, true);
}
