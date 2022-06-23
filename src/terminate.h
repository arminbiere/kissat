#ifndef _terminate_h_INCLUDED
#define _terminate_h_INCLUDED

#include "internal.h"

#ifndef QUIET
void kissat_report_termination (kissat *, const char *name,
				const char *file, long lineno,
				const char *fun);
#endif

static inline bool
kissat_terminated (kissat * solver, int bit, const char *name,
		   const char *file, long lineno, const char *fun)
{
  assert (0 <= bit), assert (bit < 64);
#ifdef COVERAGE
  const uint64_t mask = (uint64_t) 1 << bit;
  if (!(solver->termination.flagged & mask))
    return false;
  solver->termination.flagged = ~(uint64_t) 0;
#else
  if (!solver->termination.flagged)
    return false;
#endif
#ifndef QUIET
  kissat_report_termination (solver, name, file, lineno, fun);
#else
  (void) file;
  (void) fun;
  (void) lineno;
  (void) name;
#endif
#if !defined (COVERAGE) && defined(NDEBUG)
  (void) bit;
#endif
  return true;
}

#define TERMINATED(BIT) \
  kissat_terminated (solver, BIT, #BIT, __FILE__, __LINE__, __func__)

#define backbone_terminated_1 0
#define backbone_terminated_2 1
#define backbone_terminated_3 2
#define eliminate_terminated_1 3
#define eliminate_terminated_2 4
#define forward_terminated_1 5
#define kitten_terminated_1 6
#define search_terminated_1 7
#define substitute_terminated_1 8
#define sweep_terminated_1 9
#define sweep_terminated_2 10
#define sweep_terminated_3 11
#define sweep_terminated_4 12
#define sweep_terminated_5 13
#define sweep_terminated_6 14
#define sweep_terminated_7 15
#define vivify_terminated_1 16
#define vivify_terminated_2 17
#define walk_terminated_1 18
#define warmup_terminated_1 19

#endif
