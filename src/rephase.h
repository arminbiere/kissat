#ifndef _rephase_h_INCLUDED
#define _rephase_h_INCLUDED

#include <stdbool.h>
#include <stdint.h>

struct kissat;

bool kissat_rephasing (struct kissat *);
void kissat_rephase (struct kissat *);
char kissat_rephase_best (struct kissat *);

#if 0
void kissat_reset_best_assigned (struct kissat *);
void kissat_reset_target_assigned (struct kissat *);
#endif

#endif
