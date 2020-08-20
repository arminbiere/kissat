#ifndef _propdense_h_INCLUDED
#define _propdense_h_INCLUDED

struct kissat;

#include <limits.h>

#define NO_DENSE_PROPAGATION_LIMIT UINT_MAX

bool kissat_dense_propagate (struct kissat *,
			     unsigned limit, unsigned ignore_idx);

#endif
