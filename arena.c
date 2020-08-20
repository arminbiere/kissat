#include "error.h"
#include "internal.h"
#include "logging.h"
#include "print.h"

static void
report_resized (kissat * solver, const char *mode, arena before)
{
#ifndef QUIET
  const word *old_begin = BEGIN_STACK (before);
  const word *new_begin = BEGIN_STACK (solver->arena);
  const bool moved = (new_begin != old_begin);
  const uint64_t capacity = CAPACITY_STACK (solver->arena);
  const uint64_t bytes = capacity * sizeof (word);
  kissat_phase (solver, "arena", GET (arena_resized),
		"%s to %s words %s (%s)", mode, FORMAT_COUNT (capacity),
		FORMAT_BYTES (bytes), (moved ? "moved" : "in place"));
#else
  (void) solver;
  (void) mode;
  (void) before;
#endif
}

reference
kissat_allocate_clause (kissat * solver, size_t size)
{
  assert (size <= UINT_MAX);
  const size_t res = SIZE_STACK (solver->arena);
  assert (res <= MAX_REF);
  const size_t bytes = kissat_bytes_of_clause (size);
  assert (kissat_aligned_word (bytes));
  const size_t needed = bytes / sizeof (word);
  assert (needed <= UINT_MAX);
  size_t capacity = CAPACITY_STACK (solver->arena);
  assert (kissat_is_power_of_two (MAX_ARENA));
  assert (capacity <= MAX_ARENA);
  size_t available = capacity - res;
  if (needed > available)
    {
      const arena before = solver->arena;
      do
	{
	  assert (kissat_is_zero_or_power_of_two (capacity));
	  if (capacity == MAX_ARENA)
	    kissat_fatal ("maximum arena capacity "
			  "of 2^%d words %s exhausted",
			  LD_MAX_ARENA,
			  FORMAT_BYTES (MAX_ARENA * sizeof (word)));
	  kissat_stack_enlarge (solver, (chars *) & solver->arena,
				sizeof (word));
	  capacity = CAPACITY_STACK (solver->arena);
	  available = capacity - res;
	}
      while (needed > available);
      INC (arena_resized);
      INC (arena_enlarged);
      report_resized (solver, "enlarged", before);
      assert (capacity <= MAX_ARENA);
    }
  solver->arena.end += needed;
  LOG ("allocated clause[%u] of size %zu bytes %s",
       res, size, FORMAT_BYTES (bytes));
  return res;
}

void
kissat_shrink_arena (kissat * solver)
{
  const arena before = solver->arena;
  const size_t capacity = CAPACITY_STACK (before);
  const size_t size = SIZE_STACK (before);
#ifndef QUIET
  const size_t capacity_bytes = capacity * sizeof (word);
  kissat_phase (solver, "arena", GET (arena_resized),
		"capacity of %s words %s",
		FORMAT_COUNT (capacity), FORMAT_BYTES (capacity_bytes));
  const size_t size_bytes = size * sizeof (word);
  kissat_phase (solver, "arena", GET (arena_resized),
		"filled %.0f%% with %s words %s",
		kissat_percent (size, capacity),
		FORMAT_COUNT (size), FORMAT_BYTES (size_bytes));
#endif
  if (size > capacity / 4)
    {
      kissat_phase (solver, "arena", GET (arena_resized),
		    "not shrinking since more than 25%% filled");
      return;
    }
  INC (arena_resized);
  INC (arena_shrunken);
  SHRINK_STACK (solver->arena);
  report_resized (solver, "shrunken", before);
}

#if !defined(NDEBUG) || defined(LOGGING)

bool
kissat_clause_in_arena (const kissat * solver, const clause * c)
{
  if (!kissat_aligned_pointer (c))
    return false;
  const char *p = (char *) c;
  const char *begin = (char *) BEGIN_STACK (solver->arena);
  const char *end = (char *) END_STACK (solver->arena);
  if (p < begin)
    return false;
  const size_t bytes = kissat_bytes_of_clause (c->size);
  if (end < p + bytes)
    return false;
  return true;
}

#endif
