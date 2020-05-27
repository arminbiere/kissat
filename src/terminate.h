#ifndef _terminate_h_INCLUDED
#define _terminate_h_INCLUDED

#include "internal.h"

#ifndef QUIET
void kissat_report_termination (kissat *, int bit, const char *file,
				long lineno, const char *fun);
#endif

static inline bool
kissat_terminated (kissat * solver, int bit,
		   const char *file, long lineno, const char *fun)
{
  assert (0 <= bit), assert (bit < 32);
#ifdef COVERAGE
  const unsigned mask = 1u << bit;
  if (!(solver->terminate & mask))
    return false;
  solver->terminate = ~(unsigned) 0;
#else
  if (!solver->terminate)
    return false;
#endif
#ifndef QUIET
  kissat_report_termination (solver, bit, file, lineno, fun);
#else
  (void) file;
  (void) lineno;
  (void) fun;
#ifndef COVERAGE
  (void) bit;
#endif
#endif
  return true;
}

#define TERMINATED(BIT) \
  kissat_terminated (solver, BIT, __FILE__, __LINE__, __func__)

#endif
