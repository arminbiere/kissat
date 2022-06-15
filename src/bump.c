#include "analyze.h"
#include "bump.h"
#include "inlineheap.h"
#include "inlinequeue.h"
#include "internal.h"
#include "logging.h"
#include "print.h"
#include "rank.h"
#include "sort.h"

#define RANK(A) ((A).rank)
#define SMALLER(A,B) (RANK (A) < RANK (B))

#define RADIX_SORT_BUMP_LIMIT 32

static void
sort_bump (kissat * solver)
{
  const size_t size = SIZE_STACK (solver->analyzed);
  if (size < RADIX_SORT_BUMP_LIMIT)
    {
      LOG ("quick sorting %zu analyzed variables", size);
      SORT_STACK (datarank, solver->ranks, SMALLER);
    }
  else
    {
      LOG ("radix sorting %zu analyzed variables", size);
      RADIX_STACK (datarank, unsigned, solver->ranks, RANK);
    }
}

static void
rescale_scores (kissat * solver)
{
  INC (rescaled);
  assert (!solver->branching);
  heap *scores = &solver->scores[0];
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
bump_score_increment (kissat * solver)
{
  const double old_scinc = solver->scinc;
  const double decay = GET_OPTION (decay) * 1e-3;
  assert (0 <= decay), assert (decay <= 0.5);
  const double factor = 1.0 / (1.0 - decay);
  const double new_scinc = old_scinc * factor;
  LOG ("new score increment %g = %g * %g", new_scinc, factor, old_scinc);
  solver->scinc = new_scinc;
  if (new_scinc > MAX_SCORE)
    rescale_scores (solver);
}

static inline void
bump_analyzed_variable_score (kissat * solver, unsigned idx)
{
  assert (!solver->branching);
  heap *scores = &solver->scores[0];
  const double old_score = kissat_get_heap_score (scores, idx);
  const double inc = GET_OPTION (acids) ?
    (CONFLICTS - old_score) / 2.0 : solver->scinc;
  const double new_score = old_score + inc;
  LOG ("new score[%u] = %g = %g + %g", idx, new_score, old_score, inc);
  kissat_update_heap (solver, scores, idx, new_score);
  if (new_score > MAX_SCORE)
    rescale_scores (solver);
}

static void
bump_analyzed_variable_scores (kissat * solver)
{
  assert (!solver->branching);
  flags *flags = solver->flags;

  for (all_stack (unsigned, idx, solver->analyzed))
    if (flags[idx].active)
        bump_analyzed_variable_score (solver, idx);

  if (!GET_OPTION (acids))
    bump_score_increment (solver);
}

static void
move_analyzed_variables_to_front_of_queue (kissat * solver)
{
  assert (EMPTY_STACK (solver->ranks));
  const links *const links = solver->links;
  for (all_stack (unsigned, idx, solver->analyzed))
    {
// *INDENT-OFF*
      const datarank rank = { .data = idx, .rank = links[idx].stamp };
// *INDENT-ON*
      PUSH_STACK (solver->ranks, rank);
    }

  sort_bump (solver);

  flags *flags = solver->flags;
  unsigned idx;

  for (all_stack (datarank, rank, solver->ranks))
    if (flags[idx = rank.data].active)
      kissat_move_to_front (solver, idx);

  CLEAR_STACK (solver->ranks);
}

static void
update_conflicted (kissat * solver)
{
  const uint64_t conflicts = CONFLICTS;
  flags *flags = solver->flags;

  for (all_stack (unsigned, idx, solver->analyzed))
    if (flags[idx].active)
        solver->conflicted[idx] = conflicts;
}

void
kissat_bump_analyzed (kissat * solver)
{
  START (bump);
  const size_t bumped = SIZE_STACK (solver->analyzed);
  if (!solver->stable)
    move_analyzed_variables_to_front_of_queue (solver);
  else if (solver->branching)
    update_conflicted (solver);
  else
    bump_analyzed_variable_scores (solver);
  ADD (literals_bumped, bumped);
  STOP (bump);
}

void
kissat_bump_propagated (kissat * solver)
{
  assert (solver->branching);
  assert (GET_OPTION (bump));

  const unsigned level = solver->level;
  if (!level)
    return;

  assert (!solver->probing);

  const uint64_t conflicts = CONFLICTS;
  const double alpha = solver->alphachb;

  const uint64_t *conflicted = solver->conflicted;
  const struct assigned *assigned = solver->assigned;
  assert (solver->branching);
  heap *scores = &solver->scores[1];

  const unsigned *begin = BEGIN_STACK (solver->trail);
  const unsigned *end = END_STACK (solver->trail);
  const unsigned *t = end;

  while (t > begin)
    {
      const unsigned lit = *--t;
      const unsigned idx = IDX (lit);

      if (assigned[idx].level != level)
	break;

      const uint64_t last = conflicted[idx];
      const uint64_t diff = last ? (conflicts - last) : 0;	// UINT_MAX;
      const uint64_t age = (diff == UINT64_MAX ? UINT64_MAX : diff + 1);
      const double reward = 1.0 / age;
      const double old_score = kissat_get_heap_score (scores, idx);
      const double new_score = alpha * reward + (1 - alpha) * old_score;
      LOG ("new score[%u] = %g vs %g", idx, new_score, old_score);
      kissat_update_heap (solver, scores, idx, new_score);
    }
}

void
kissat_update_scores (kissat * solver)
{
  assert (solver->stable);
  heap *scores = SCORES;
  for (all_variables (idx))
    if (ACTIVE (idx) && !kissat_heap_contains (scores, idx))
      kissat_push_heap (solver, scores, idx);
}
