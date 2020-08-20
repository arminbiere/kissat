#ifndef _smooth_h_INCLUDED
#define _smooth_h_INCLUDED

#include <stdint.h>

typedef struct smooth smooth;

struct smooth
{
#ifdef LOGGING
  const char *name;
#endif
  double value, alpha, beta;
  uint64_t wait, period;
};

struct kissat;

void kissat_init_smooth (struct kissat *, smooth *, int window, const char *);
void kissat_update_smooth (struct kissat *, smooth *, double);

#endif
