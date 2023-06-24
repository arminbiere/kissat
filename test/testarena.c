#include "../src/allocate.h"
#include "../src/error.h"
#include "../src/resources.h"

#include "test.h"

static void test_arena_basic (void) {
  assert (sizeof (ward) >= sizeof (void *));
  assert (!(sizeof (ward) % sizeof (void *)));
  assert (kissat_aligned_word (sizeof (ward)));
  assert (kissat_aligned_pointer (0));
  DECLARE_AND_INIT_SOLVER (solver);
  unsigned size = 3;
  while (size < 20)
    kissat_allocate_clause (solver, size++);
  while (size <= (1u << 23))
    kissat_allocate_clause (solver, size *= 2);
  const int n = tissat_big ? 407 : 10;
  for (int i = 0; i < n; i++) {
    printf ("iteration %d\n", i);
    (void) kissat_allocate_clause (solver, size);
  }
  RELEASE_STACK (solver->arena);
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

static void test_arena_realloc (void) {
  format format;
  memset (&format, 0, sizeof format);
  DECLARE_AND_INIT_SOLVER (solver);
#ifndef QUIET
  const char *formatted =
      kissat_format_bytes (&format, kissat_maximum_resident_set_size ());
  printf ("before allocating arena %s\n", formatted);
#endif
  const unsigned bytes = (1u << 22);
  const unsigned size = bytes / 4 - 3;
  const unsigned n = tissat_big ? (1u << 8) : (1u << 3);
  for (unsigned i = 0; i < n; i++) {
    reference ref = kissat_allocate_clause (solver, size);
    clause *c = kissat_unchecked_dereference_clause (solver, ref);
    c->size = size;
    c->shrunken = false;
    for (unsigned i = 0; i < size; i++)
      c->lits[i] = 42;
#ifndef QUIET
    formatted =
        kissat_format_bytes (&format, kissat_maximum_resident_set_size ());
    printf ("after allocated %u size %u clause %s\n", i + 1, size,
            formatted);
#endif
  }
#ifdef METRICS
  assert (solver->statistics.allocated_current == n * bytes);
#endif
  unsigned count = 0;
  for (all_clauses (c))
    count++;
  assert (count == n);
  RELEASE_STACK (solver->arena);
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

static void test_arena_traverse (void) {
  DECLARE_AND_INIT_SOLVER (solver);
  const unsigned n = 1000;
  for (unsigned size = 3; size < n + 3; size++) {
    reference ref = kissat_allocate_clause (solver, size);
    clause *c = kissat_unchecked_dereference_clause (solver, ref);
    c->size = size;
    c->shrunken = false;
    for (unsigned i = 0; i < size; i++)
      c->lits[i] = size;
  }
  unsigned found = 0, size = 3;
  for (all_clauses (c)) {
    found++;
    assert (c->size == size);
    assert (!c->shrunken);
    assert (c->lits[0] == size);
    size++;
  }
  assert (found == n);
  RELEASE_STACK (solver->arena);
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

#include <setjmp.h>

static jmp_buf jump_buffer;

static void abort_call_back (void) { longjmp (jump_buffer, 42); }

static void test_arena_fatal (void) {
  kissat_call_function_instead_of_abort (abort_call_back);
  int val = setjmp (jump_buffer);
  if (val) {
    kissat_call_function_instead_of_abort (0);
    if (val != 42)
      FATAL ("expected '42' as result from 'setjmp'");
  } else {
    {
      DECLARE_AND_INIT_SOLVER (solver);
      arena *arena = &solver->arena;
      ;
      arena->end = arena->begin + MAX_ARENA - 1;
      arena->allocated = arena->end + 1;
      tissat_verbose ("size of arena-word: %zu bytes", sizeof (ward));
      tissat_verbose ("maximum arena size: %zu = %s", (size_t) MAX_ARENA,
                      FORMAT_BYTES (MAX_ARENA * sizeof (ward)));
      size_t size = SIZE_STACK (*arena);
      size_t capacity = CAPACITY_STACK (*arena);
      tissat_verbose ("size of arena: %zu = %s", size,
                      FORMAT_BYTES (size * sizeof (ward)));
      tissat_verbose ("capacity of arena: %zu = %s", capacity,
                      FORMAT_BYTES (capacity * sizeof (ward)));
      assert (capacity == MAX_ARENA);
      assert (size + 1 == MAX_ARENA);
      kissat_allocate_clause (solver, 3);
    }
    kissat_call_function_instead_of_abort (0);
    FATAL ("long jump not taken");
  }
}

void tissat_schedule_arena (void) {
  SCHEDULE_FUNCTION (test_arena_basic);
  SCHEDULE_FUNCTION (test_arena_realloc);
  SCHEDULE_FUNCTION (test_arena_traverse);
  SCHEDULE_FUNCTION (test_arena_fatal);
}
