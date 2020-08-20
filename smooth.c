#include "allocate.h"
#include "internal.h"
#include "logging.h"

#include <inttypes.h>

void
kissat_init_smooth (kissat * solver, smooth * smooth, int window,
		    const char *name)
{
  assert (window > 0);
  const double alpha = 1.0 / window;
  LOG ("initialized 'EMA (%s)' with alpha %g (window %d)", name, alpha,
       window);
  smooth->value = 0;
  smooth->alpha = alpha;
  smooth->beta = 1.0;
  smooth->wait = smooth->period = 0;
#ifdef LOGGING
  smooth->name = name;
#else
  (void) solver;
  (void) name;
#endif
}

void
kissat_update_smooth (kissat * solver, smooth * smooth, double y)
{
  smooth->value += smooth->beta * (y - smooth->value);
  LOG ("updated 'EMA (%s)' with %g (%s %g) yields %g",
       smooth->name,
       (smooth->beta == smooth->alpha ? "alpha" : "beta"),
       y, smooth->beta, smooth->value);
  if (smooth->beta <= smooth->alpha || smooth->wait--)
    return;
  smooth->wait = smooth->period = 2 * (smooth->period + 1) - 1;
  smooth->beta *= 0.5;
  if (smooth->beta < smooth->alpha)
    smooth->alpha = smooth->beta;
  LOG ("new EMA (%s) wait = period = %" PRIu64 ", beta = %g",
       smooth->name, smooth->wait, smooth->beta);
#ifndef LOGGING
  (void) solver;
#endif
}
