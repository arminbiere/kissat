#include "bump.h"
#include "internal.h"
#include "logging.h"
#include "print.h"
#include "rank.h"
#include "sort.h"

static inline unsigned
rank_of_idxrank (idxrank ir)
{
  return ir.rank;
}

static inline bool
smaller_idxrank (idxrank ir, idxrank jr)
{
  return ir.rank < jr.rank;
}

#define RADIX_SORT_BUMP_LIMIT 800
#define RADIX_SORT_BUMP_LENGTH 8

static void
sort_bump (kissat * solver)
{
  const size_t size = SIZE_STACK (solver->analyzed);
  if (size < RADIX_SORT_BUMP_LIMIT)
    {
      LOG ("quick sorting %zu analyzed variables", size);
      SORT_STACK (idxrank, solver->bump, smaller_idxrank);
    }
  else
    {
      LOG ("radix sorting %zu analyzed variables", size);
      RADIX_STACK (RADIX_SORT_BUMP_LENGTH, idxrank,
		   unsigned, solver->bump, rank_of_idxrank);
    }
}

static void
rescale_scores (kissat * solver, heap * scores)
{
  INC (rescaled);
  const double max_score = kissat_max_score_on_heap (scores);
  kissat_phase (solver, "rescale", GET (rescaled),
		"maximum score %g increment %g", max_score, solver->scinc);
  const double rescale = MAX (max_score, solver->scinc);
  assert (rescale > 0);
  const double factor = 1.0 / rescale;
  kissat_rescale_heap (solver, scores, factor);
  solver->scinc *= factor;
  kissat_phase (solver, "rescale",
		GET (rescaled), "rescaled by factor %g", factor);
}

static void
bump_score_increment (kissat * solver, heap * scores)
{
  const double old_scinc = solver->scinc;
  const double decay = GET_OPTION (decay) * 1e-3;
  assert (0 <= decay), assert (decay <= 0.5);
  const double factor = 1.0 / (1.0 - decay);
  const double new_scinc = old_scinc * factor;
  LOG ("new score increment %g = %g * %g", new_scinc, factor, old_scinc);
  solver->scinc = new_scinc;
  if (new_scinc > MAX_SCORE)
    rescale_scores (solver, scores);
}

static inline void
bump_variable_score (kissat * solver, heap * scores, unsigned idx)
{
  const double old_score = kissat_get_heap_score (scores, idx);
  const double new_score = old_score + solver->scinc;
  LOG ("new score[%u] = %g = %g + %g",
       idx, new_score, old_score, solver->scinc);
  kissat_update_heap (solver, scores, idx, new_score);
  if (new_score > MAX_SCORE)
    rescale_scores (solver, scores);
}

static void
bump_analyzed_variable_scores (kissat * solver)
{
  heap *scores = &solver->scores;
  flags *flags = solver->flags;

  for (all_stack (unsigned, idx, solver->analyzed))
    if (flags[idx].active)
        bump_variable_score (solver, scores, idx);

  bump_score_increment (solver, scores);
}

static void
move_analyzed_variables_to_front_of_queue (kissat * solver)
{
  assert (EMPTY_STACK (solver->bump));
  const links *links = solver->links;
  for (all_stack (unsigned, idx, solver->analyzed))
    {
// *INDENT-OFF*
    const idxrank idxrank = { .idx = idx, .rank = links[idx].stamp };
// *INDENT-ON*
      PUSH_STACK (solver->bump, idxrank);
    }

  sort_bump (solver);

  flags *flags = solver->flags;
  unsigned idx;

  for (all_stack (idxrank, idxrank, solver->bump))
    if (flags[idx = idxrank.idx].active)
      kissat_move_to_front (solver, idx);

  CLEAR_STACK (solver->bump);
}

void
kissat_bump_variables (kissat * solver)
{
  START (bump);
  assert (!solver->probing);
  if (solver->stable)
    bump_analyzed_variable_scores (solver);
  else
    move_analyzed_variables_to_front_of_queue (solver);
  STOP (bump);
}
