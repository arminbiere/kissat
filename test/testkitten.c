#include "test.h"

#ifdef KITTEN

#include "../src/file.h"

#include "testcnfs.h"

static void schedule_solve_kitten (int expected, const char *kitten,
                                   const char *name) {
  char *cmd = malloc (strlen (kitten) + strlen (name) + 32);
  sprintf (cmd, "%s ../test/cnf/%s.cnf", kitten, name);
  tissat_schedule_command (expected, cmd, 0);
  free (cmd);
}

static void schedule_solving_kitten (const char *kitten) {
#define CNF(EXPECTED, NAME, BIG) \
  if (!BIG || tissat_big) \
    schedule_solve_kitten (EXPECTED, kitten, #NAME);
  CNFS
#undef CNF
}

static void schedule_core_kitten (const char *kitten, const char *name) {
  char *cmd = malloc (2 * strlen (kitten) + 2 * strlen (name) + 64);
  sprintf (cmd, "%s -O2 ../test/cnf/%s.cnf %s/%s.core", kitten, name,
           tissat_root, name);
  tissat_job *reduce = tissat_schedule_command (20, cmd, 0);
  sprintf (cmd, "%s %s/%s.core", kitten, tissat_root, name);
  tissat_schedule_command (20, cmd, reduce);
  free (cmd);
}

static void schedule_cores_kitten (const char *kitten) {
#define CNF(EXPECTED, NAME, BIG) \
  if (EXPECTED == 20 && (!BIG || tissat_big)) \
    schedule_core_kitten (kitten, #NAME);
  CNFS
#undef CNF
}

#endif

#include "../src/kitten.h"

static void test_kitten_tie_and_shirt (void) {
  DECLARE_AND_INIT_SOLVER (solver);
  kitten *kitten = kitten_embedded (solver);
  const unsigned tie = 0, not_tie = 1;
  const unsigned shirt = 2, not_shirt = 3;
  kitten_binary (kitten, tie, shirt);
  kitten_binary (kitten, not_tie, shirt);
  kitten_binary (kitten, not_tie, not_shirt);
  int res = kitten_solve (kitten);
  assert (res == 10);
  (void) res;
  assert (kitten_value (kitten, tie) < 0);
  assert (kitten_value (kitten, shirt) > 0);
  assert (kitten_value (kitten, not_tie) > 0);
  assert (kitten_value (kitten, not_shirt) < 0);
  kitten_release (kitten);
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

static void test_kitten_assumption_one_clause (void) {
  DECLARE_AND_INIT_SOLVER (solver);
  kitten *kitten = kitten_embedded (solver);
#define N 8
  for (unsigned int n = 0; n < N; n++) {
    printf ("testing %u kitten assumptions\n", n);
    unsigned clause[N], size = 0;
    for (unsigned idx = 0; idx < n; idx += 2)
      clause[size++] = 2 * idx;
    kitten_clause (kitten, size, clause);
    int res = kitten_solve (kitten);
    if (n) {
      assert (res == 10);

      bool found = false;
      for (unsigned lit = 0; lit < 2 * n; lit += 4)
        if (kitten_value (kitten, lit) > 0)
          found = true;
      assert (found);
      (void) found;

      for (unsigned bits = 0; bits < (1u << n); bits++) {
        bool assumed_even = false;
        for (unsigned idx = 0; idx < n; idx++) {
          unsigned lit = 2 * idx;
          if (bits & (1 << idx))
            lit ^= 1;
          else if (!(idx & 1))
            assumed_even = true;
          kitten_assume (kitten, lit);
        }

        res = kitten_solve (kitten);
        if (assumed_even)
          assert (res == 10);
        else
          assert (res == 20);
      }
    } else
      assert (res == 20);
    (void) res;
    kitten_clear (kitten);
  }
  kitten_release (kitten);
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

void tissat_schedule_kitten (void) {
  SCHEDULE_FUNCTION (test_kitten_tie_and_shirt);
  SCHEDULE_FUNCTION (test_kitten_assumption_one_clause);

#ifdef KITTEN
  char *kitten = malloc (strlen (tissat_root) + 16);
  sprintf (kitten, "%s/kitten", tissat_root);
  if (kissat_file_readable (kitten)) {
    schedule_solving_kitten (kitten);
    schedule_cores_kitten (kitten);
  }
  free (kitten);
#endif
}
