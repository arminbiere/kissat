#ifndef _inlinescore_h_INCLUDED
#define _inlinescore_h_INCLUDED

#include "inlineheap.h"

static inline double
kissat_variable_score (kissat * solver, unsigned idx)
{
  const unsigned lit = LIT (idx);
  const unsigned not_lit = NOT (lit);
  size_t pos = SIZE_WATCHES (WATCHES (lit));
  size_t neg = SIZE_WATCHES (WATCHES (not_lit));
  return (double) pos *neg + pos + neg;
}

static inline void
kissat_update_variable_score (kissat * solver, heap * schedule, unsigned idx)
{
  assert (schedule->size);
  double new_score = kissat_variable_score (solver, idx);
  LOG ("new score %g for variable %s", new_score, LOGVAR (idx));
  kissat_update_heap (solver, schedule, idx, -new_score);
}

#endif
