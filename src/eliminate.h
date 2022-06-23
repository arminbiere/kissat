#ifndef _eliminate_hpp_INCLUDED
#define _eliminate_hpp_INCLUDED

#include <stdbool.h>

struct kissat;
struct clause;

void kissat_flush_units_while_connected (struct kissat *);

bool kissat_eliminating (struct kissat *);
int kissat_eliminate (struct kissat *);

void kissat_eliminate_binary (struct kissat *, unsigned, unsigned);

#endif
