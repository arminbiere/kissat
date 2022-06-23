#ifndef _assign_h_INCLUDED
#define _assign_h_INCLUDED

#include <stdbool.h>

#define DECISION_REASON	UINT_MAX
#define UNIT_REASON	(DECISION_REASON - 1)

#define INVALID_LEVEL UINT_MAX

typedef struct assigned assigned;
struct clause;

struct assigned
{
  unsigned level;
  unsigned trail;

  unsigned analyzed:1;
  unsigned binary:1;
  unsigned poisoned:1;
  unsigned redundant:1;
  unsigned removable:1;
  unsigned shrinkable:1;

  unsigned reason;
};

#define ASSIGNED(LIT) \
  (assert (VALID_INTERNAL_LITERAL (LIT)), \
   solver->assigned + IDX (LIT))

#define LEVEL(LIT) \
  (ASSIGNED(LIT)->level)

#define REASON(LIT) \
  (ASSIGNED(LIT)->reason)

#ifndef FAST_ASSIGN

#include "reference.h"

struct kissat;
struct clause;

void kissat_assign_unit (struct kissat *, unsigned lit, const char *);
void kissat_learned_unit (struct kissat *, unsigned lit);
void kissat_original_unit (struct kissat *, unsigned lit);

void kissat_assign_decision (struct kissat *, unsigned lit);

void kissat_assign_binary (struct kissat *, bool, unsigned, unsigned);

void kissat_assign_reference (struct kissat *, unsigned lit,
			      reference, struct clause *);

#endif

#endif
