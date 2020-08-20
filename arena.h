#ifndef _arena_h_INCLUDED
#define _arena_h_INCLUDED

#include "reference.h"
#include "stack.h"
#include "utilities.h"

#define LD_MAX_ARENA ((sizeof (word) == 4) ? 28 : LD_MAX_REF)
#define MAX_ARENA ((size_t)1 << LD_MAX_ARENA)

// *INDENT-OFF*

typedef STACK (word) arena;

// *INDENT-ON*

struct clause;
struct kissat;

reference kissat_allocate_clause (struct kissat *, size_t size);
void kissat_shrink_arena (struct kissat *);

#if !defined(NDEBUG) || defined(LOGGING)

bool kissat_clause_in_arena (const struct kissat *, const struct clause *);

#endif

#endif
