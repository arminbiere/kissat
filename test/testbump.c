#include "../src/bump.h"

#include "test.h"

void kissat_update_scores (kissat *);

static void test_bump_rescale (void) {
  kissat *solver = kissat_init ();
  kissat_add (solver, 1);
  kissat_add (solver, 2);
  kissat_add (solver, 0);
  kissat_add (solver, -1);
  kissat_add (solver, -2);
  kissat_add (solver, 0);
  if (!solver->stable) {
    tissat_verbose ("forced switching to stable mode");
    solver->stable = true;
  }
  tissat_verbose ("forced updating of scores");
  kissat_update_scores (solver);
  assert (solver->scinc > 0);
  tissat_verbose ("initial score increment %g", solver->scinc);
  ACTIVE (0) = ACTIVE (1) = true;
  heap *scores = SCORES;
  unsigned count = 0;
  for (unsigned i = 1; i <= 5; i++) {
    double prev = 0;
    assert (prev < solver->scinc);
    while (prev < solver->scinc) {
      prev = solver->scinc;
      if (i != 3) {
        PUSH_STACK (solver->analyzed, 0);
        if (count++ & 1)
          PUSH_STACK (solver->analyzed, 1);
      }
      kissat_bump_analyzed (solver);
      CLEAR_STACK (solver->analyzed);
      if (prev >= solver->scinc || solver->scinc >= MAX_SCORE * 0.7 ||
          kissat_get_heap_score (scores, 0) >= MAX_SCORE * 0.7 ||
          kissat_get_heap_score (scores, 1) >= MAX_SCORE * 0.7)
        tissat_verbose ("%u.%u: score[0]=%g score[1]=%g scinc=%g", i, count,
                        kissat_get_heap_score (scores, 0),
                        kissat_get_heap_score (scores, 1), solver->scinc);
    }
  }
  kissat_release (solver);
}

void tissat_schedule_bump (void) { SCHEDULE_FUNCTION (test_bump_rescale); }
