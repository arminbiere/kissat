#include "../src/allocate.h"

#include "test.h"

static void test_references_layout (void) {
  printf ("MAX_REF                      %08x\n", MAX_REF);
  printf ("INVALID_REF                  %08x\n", INVALID_REF);
  printf ("EXTERNAL_MAX_VAR             %08x\n", EXTERNAL_MAX_VAR);
  printf ("INTERNAL_MAX_VAR             %08x\n", INTERNAL_MAX_VAR);
  printf ("INTERNAL_MAX_LIT             %08x\n", INTERNAL_MAX_LIT);
  printf ("INVALID_LIT                  %08x\n", INVALID_LIT);

  assert (sizeof (watch) == 4);

  watch w;
  memset (&w, 0, sizeof (w));

  assert (!w.type.binary);

  assert (!w.binary.binary);
  assert (!w.binary.lit);

  assert (!w.large.binary);
  assert (!w.large.ref);

  assert (!w.blocking.binary);
  assert (!w.blocking.lit);

  w.type.binary = true;

  assert (w.binary.binary);
  assert (!w.binary.lit);

  assert (w.large.binary);
  assert (!w.large.ref);

  assert (w.blocking.binary);
  assert (!w.blocking.lit);

  w.binary.lit = INTERNAL_MAX_LIT;

  assert (w.binary.lit == INTERNAL_MAX_LIT);
  assert (w.blocking.lit == INTERNAL_MAX_LIT);

  assert (w.raw != INVALID_REF);
  assert (w.raw != INVALID_VECTOR_ELEMENT);

  printf ("(true,true,INTERNAL_MAX_LIT) %08x\n", w.raw);

  w.type.binary = false;
  w.type.lit = 42;

  assert (w.binary.lit == 42);
  assert (w.blocking.lit == 42);

  w.large.ref = MAX_REF;
  assert (!w.type.binary);
  assert (w.large.ref == MAX_REF);
  assert (w.raw == MAX_REF);

  assert (w.raw != INVALID_REF);
  assert (w.raw != INVALID_VECTOR_ELEMENT);

  w.raw = MAX_REF;
  assert (!w.type.binary);
  assert (w.large.ref == MAX_REF);

  assert (w.raw != INVALID_REF);
  assert (w.raw != INVALID_VECTOR_ELEMENT);

  w.raw = INVALID_REF;
  assert (w.type.binary);
  assert (w.binary.lit > INTERNAL_MAX_LIT);

  printf ("w.large.ref (INVALID_REF)    %08x\n", w.large.ref);
  printf ("w.binary.lit (INVALID_REF)   %08x\n", w.binary.lit);
  printf ("w.blocking.lit (INVALID_REF) %08x\n", w.blocking.lit);
}

static void test_references_enlarge (void) {
  DECLARE_AND_INIT_SOLVER (solver);

  bool refs[2];
  bool found[6];

  solver->size = solver->vars = 1;

  vector watches[2];
  memset (watches, 0, sizeof watches);
  solver->watches = watches;

#define SETUP_FOUND_AND_CLAUSES() \
  do { \
    memset (found, 0, sizeof found); \
    memset (refs, 0, sizeof refs); \
  } while (0)

  kissat_push_binary_watch (solver, watches, 0);
  kissat_push_blocking_watch (solver, watches, 1, 0);
  kissat_push_binary_watch (solver, watches, 2);
  kissat_push_blocking_watch (solver, watches, 3, 1);
  kissat_push_binary_watch (solver, watches, 4);
  kissat_push_binary_watch (solver, watches, 5);

  SETUP_FOUND_AND_CLAUSES ();

  int binary = 0, blocking = 0, count = 0;

  {
    reference ref;
    for (all_binary_blocking_watch_ref (watch, ref, *watches)) {
      count++;
      if (watch.type.binary) {
        binary++;
        assert (ref == INVALID_REF);
      } else {
        blocking++;
        assert (ref < 2);
        assert (!refs[ref]);
        refs[ref] = true;
      }

      assert (watch.type.lit < 6);
      assert (!found[watch.type.lit]);
      found[watch.type.lit] = true;
    }
  }

  assert (count == 6);
  assert (binary == 4);
  assert (blocking == 2);

  for (int i = 0; i < 6; i++)
    assert (found[i]);

  for (int i = 0; i < 2; i++)
    assert (refs[i]);

  {
    watch *q = BEGIN_WATCHES (*watches);
    for (all_binary_blocking_watches (watch, *watches))
      if (watch.type.binary)
        *q++ = watch;
    SET_END_OF_WATCHES (*watches, q);
  }

  SETUP_FOUND_AND_CLAUSES ();
  count = binary = 0;

  {
    reference ref;
    for (all_binary_blocking_watch_ref (watch, ref, *watches)) {
      count++;
      assert (watch.type.binary);
      binary++;
      assert (ref == INVALID_REF);
      assert (watch.type.lit < 6);
      assert (!found[watch.type.lit]);
      found[watch.type.lit] = true;
    }
  }

  assert (count == 4);
  assert (binary == 4);

  for (int i = 0; i < 2; i++)
    assert (!refs[i]);

  assert (found[0]);
  assert (!found[1]);
  assert (found[2]);
  assert (!found[3]);
  assert (found[4]);
  assert (found[5]);

  assert (!refs[0]);
  assert (!refs[1]);

  solver->watching = false;

  kissat_push_large_watch (solver, watches, 0);
  kissat_push_large_watch (solver, watches, 1);

  SETUP_FOUND_AND_CLAUSES ();
  int large = 0;
  count = binary = 0;

  for (all_binary_large_watches (watch, *watches)) {
    count++;
    if (watch.type.binary) {
      binary++;
      assert (watch.binary.lit < 6);
      assert (!found[watch.binary.lit]);
      found[watch.binary.lit] = true;
    } else {
      large++;
      assert (watch.large.ref < 2);
      assert (!refs[watch.large.ref]);
      refs[watch.large.ref] = true;
    }
  }

  assert (count == 6);
  assert (binary == 4);
  assert (large == 2);

  assert (found[0]);
  assert (!found[1]);
  assert (found[2]);
  assert (!found[3]);
  assert (found[4]);
  assert (found[5]);

  assert (refs[0]);
  assert (refs[1]);

  RELEASE_WATCHES (*watches);
  RELEASE_STACK (solver->vectors.stack);

  solver->watches = 0;
  solver->size = 0;

#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

#undef SETUP_FOUND_AND_CLAUSES
#undef PUSH_WATCHS

void tissat_schedule_references (void) {
  SCHEDULE_FUNCTION (test_references_layout);
  SCHEDULE_FUNCTION (test_references_enlarge);
}
