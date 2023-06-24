#define TEST_VECTOR

#include "../src/allocate.h"
#include "../src/error.h"

#include <inttypes.h>

#include "test.h"

static void test_vector_basics (void) {
  DECLARE_AND_INIT_SOLVER (solver);
#define N 10
  assert (!(N & 1));
  unsigned count[N];
  vector vector[N];
  solver->size = solver->vars = N / 2;
  solver->watches = vector;
  memset (count, 0, sizeof count);
  memset (vector, 0, sizeof vector);
  srand (42);
  unsigned pushed = 0, popped = 0, defrags = 0;
  vectors *vectors = &solver->vectors;
  unsigneds *stack = &vectors->stack;
  for (unsigned i = 0; i < 100; i++) {
    if (!(i % (2 * N))) {
      kissat_defrag_vectors (solver, N, vector);
      defrags++;
    } else {
      unsigned j = rand () % N;
      if (rand () % 3) {
        printf ("%u: push %u\n", i, j);
        kissat_push_vectors (solver, &vector[j], j);
        assert (count[j] < UINT_MAX);
        count[j]++;
        pushed++;
      } else if (!kissat_empty_vector (vector + j)) {
        printf ("%u: pop %u\n", i, j);
        assert (count[j] > 0);
        unsigned tmp = *kissat_last_vector_pointer (solver, &vector[j]);
        kissat_pop_vector (solver, &vector[j]);
        assert (tmp == j);
        count[j]--;
        popped++;
      }
      assert (count[j] == kissat_size_vector (vector + j));
    }

    if (pushed)
      assert (!vectors->stack.begin[0]);

    unsigned free = 0;
    printf ("vectors[%zu]", SIZE_STACK (*stack));
    for (all_stack (unsigned, e, *stack)) {
      if (e == INVALID_REF) {
        printf (" -");
        free++;
      } else
        printf (" %u", e);
    }
    printf ("\nfree %u\n", free);
    printf ("usable %zu\n", solver->vectors.usable);
    assert (free == solver->vectors.usable);

    for (unsigned k = 0; k < N; k++) {
      size_t offset = kissat_offset_vector (solver, vector + k);
      size_t size = kissat_size_vector (vector + k);
      printf ("vector(%u) %zu[%zu]", k, offset, size);
      unsigned c = 0;
      for (all_vector (u, vector[k])) {
        printf (" %u", u);
        if (u != k) {
          assert (u == k);
        }
        c++;
      }
      printf ("\n");
      assert (c == count[k]);
    }
  }
#undef N
  printf ("pushed %u elements\n", pushed);
  printf ("popped %u elements\n", popped);
  printf ("defrag %u times\n", defrags);
#ifndef QUIET
  RELEASE_STACK (solver->profiles.stack);
#endif
  RELEASE_STACK (*stack);
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

#include <setjmp.h>

static jmp_buf jump_buffer;

static void abort_call_back (void) { longjmp (jump_buffer, 42); }

static void test_vector_fatal (void) {
  kissat_call_function_instead_of_abort (abort_call_back);
  int val = setjmp (jump_buffer);
  if (val) {
    kissat_call_function_instead_of_abort (0);
    if (val != 42)
      FATAL ("expected '42' as result from 'setjmp'");
  } else {
    {
      DECLARE_AND_INIT_SOLVER (solver);
      vectors *vectors = &solver->vectors;
      ;
      vectors->stack.end = vectors->stack.begin + MAX_VECTORS;
      vectors->stack.allocated = vectors->stack.end;
      assert (CAPACITY_STACK (vectors->stack) == MAX_VECTORS);
      assert (SIZE_STACK (vectors->stack) == MAX_VECTORS);
      vector vector;
      memset (&vector, 0, sizeof vector);
      kissat_enlarge_vector (solver, &vector);
    }
    kissat_call_function_instead_of_abort (0);
    FATAL ("long jump not taken");
  }
}

void tissat_schedule_vector (void) {
  SCHEDULE_FUNCTION (test_vector_basics);
  SCHEDULE_FUNCTION (test_vector_fatal);
}
