#ifndef _branching_h_INCLUDED
#define _branching_h_INCLUDED

#include <stdbool.h>

struct kissat;

void kissat_init_branching (struct kissat *);
bool kissat_toggle_branching (struct kissat *);

#endif
