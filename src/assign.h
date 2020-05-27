#ifndef _assign_h_INCLUDED
#define _assign_h_INCLUDED

#include <stdbool.h>

#define DECISION UINT_MAX
#define UNIT (DECISION - 1)

#define ANALYZED 1
#define POISONED 2
#define REMOVABLE 3

#define LD_MAX_LEVEL 28
#define INFINITE_LEVEL UINT_MAX
#define INVALID_LEVEL UINT_MAX
#define MAX_LEVEL ((1u << LD_MAX_LEVEL) - 1)

typedef struct assigned assigned;
struct clause;

struct assigned
{
  unsigned level:LD_MAX_LEVEL;
  unsigned analyzed:2;
  bool redundant:1;
  bool binary:1;
  unsigned reason;
};

#define ASSIGNED(LIT) \
  (assert (VALID_INTERNAL_LITERAL (LIT)), \
   solver->assigned + IDX (LIT))

#define LEVEL(LIT) \
  (ASSIGNED(LIT)->level)

#define REASON(LIT) \
  (ASSIGNED(LIT)->reason)

#ifndef INLINE_ASSIGN

struct kissat;
struct clause;

void kissat_assign_unit (struct kissat *, unsigned lit);
void kissat_assign_decision (struct kissat *, unsigned lit);

void kissat_assign_binary (struct kissat *, bool, unsigned, unsigned);

void kissat_assign_reference (struct kissat *, unsigned lit,
			      reference, struct clause *);

#endif

#endif
