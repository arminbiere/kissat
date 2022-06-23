#ifndef _bump_h_INCLUDED
#define _bump_h_INCLUDED

#include <stdbool.h>

struct kissat;

void kissat_bump_analyzed (struct kissat *);
void kissat_update_scores (struct kissat *);

#define MAX_SCORE 1e150

#endif
