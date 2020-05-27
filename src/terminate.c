#include "print.h"
#include "terminate.h"

#ifndef QUIET

void
kissat_report_termination (kissat * solver, int bit,
			   const char *file, long lineno, const char *fun)
{
  kissat_very_verbose (solver, "%s:%ld: %s: TERMINATED (%d)",
		       file, lineno, fun, bit);
}

#else
int kissat_terminate_dummy_to_avoid_warning;
#endif
