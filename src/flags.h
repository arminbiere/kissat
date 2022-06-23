#ifndef _flags_h_INCLUDED
#define _flags_h_INCLUDED

#include <stdbool.h>

typedef struct flags flags;

struct flags
{
	unsigned active:1;
	unsigned backbone0:1;
	unsigned backbone1:1;
	unsigned eliminate:1;
	unsigned eliminated:1;
	unsigned fixed:1;
	unsigned subsume:1;
	unsigned sweep:1;
};

#define FLAGS(IDX) \
  (assert ((IDX) < VARS), (solver->flags + (IDX)))

#define ACTIVE(IDX) (FLAGS(IDX)->active)
#define ELIMINATED(IDX) (FLAGS(IDX)->eliminated)

struct kissat;

void kissat_activate_literal (struct kissat *, unsigned);
void kissat_activate_literals (struct kissat *, unsigned, unsigned *);

void kissat_mark_eliminated_variable (struct kissat *, unsigned idx);
void kissat_mark_fixed_literal (struct kissat *, unsigned lit);

void kissat_mark_added_literals (struct kissat *, unsigned, unsigned *);
void kissat_mark_removed_literals (struct kissat *, unsigned, unsigned *);

#endif
