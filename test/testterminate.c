#ifdef COVERAGE

#include "../src/parse.h"
#include "../src/terminate.h"

#include "test.h"

static void
test_terminate (int bit, const char *name,
		bool walkinitially,
		int probeinit, int eliminateinit, int rephaseint,
		const char *cnf)
{
#ifdef NOPTIONS
  if (walkinitially)
    return;
  if (probeinit >= 0)
    return;
  if (eliminateinit >= 0)
    return;
  if (rephaseint >= 0)
    return;
#endif
  kissat *solver = kissat_init ();
  tissat_init_solver (solver);
#ifndef NOPTIONS
  if (rephaseint > 0)
    solver->options.rephaseinit = solver->options.rephaseint = rephaseint;
  if (walkinitially)
    solver->options.walkinitially = true;
  if (probeinit >= 0)
    solver->options.probeinit = probeinit;
  if (eliminateinit >= 0)
    solver->options.eliminateinit = eliminateinit;
#endif
  file file;
  const bool opened = kissat_open_to_read_file (&file, cnf);
  if (!opened)
    FATAL ("could not read '%s'", cnf);
  tissat_verbose ("parsing '%s'", cnf);
  uint64_t lineno;
  int max_var;
  const char *error =
    kissat_parse_dimacs (solver, RELAXED_PARSING, &file, &lineno, &max_var);
  if (error)
    FATAL ("unexpected parse error: %s", error);
  (void) error;
  kissat_close_file (&file);
  tissat_verbose ("solving '%s' forcing 'TERMINATED (%s)'", cnf, name);
  assert (0 <= bit), assert (bit < 32);
  solver->termination.flagged = (1u << bit);
  int res = kissat_solve (solver);
  if (res)
    FATAL ("solver returned '%d' but expected '0'", res);
  else
    tissat_verbose ("solver returned '0' as expected");
  kissat_release (solver);
}

// *INDENT-OFF*

#define TEST_TERMINATE_BITS \
TEST_TERMINATE (autarky_terminated_1,false,-1,-1,-1,"ph11") \
TEST_TERMINATE (autarky_terminated_2,false,-1,-1,-1,"ph11") \
TEST_TERMINATE (autarky_terminated_3,false,-1,-1,-1,"ph11") \
TEST_TERMINATE (autarky_terminated_4,false,-1,-1,-1,"ph11") \
TEST_TERMINATE (backbone_terminated_1,false,0,-1,-1,"add8") \
TEST_TERMINATE (backbone_terminated_2,false,0,-1,-1,"add8") \
TEST_TERMINATE (backbone_terminated_3,false,0,-1,-1,"add8") \
TEST_TERMINATE (eliminate_terminated_1,false,-1,0,-1,"add8") \
TEST_TERMINATE (failed_terminated_1,false,0,-1,-1,"add8") \
TEST_TERMINATE (failed_terminated_2,false,0,-1,-1,"add8") \
TEST_TERMINATE (forward_terminated_1,false,-1,0,-1,"add8") \
TEST_TERMINATE (rephase_terminated_1,false,-1,-1,-1,"ph11") \
TEST_TERMINATE (rephase_terminated_2,false,-1,-1,-1,"ph11") \
TEST_TERMINATE (search_terminated_1,false,-1,-1,-1,"add8") \
TEST_TERMINATE (substitute_terminated_1,false,0,-1,-1,"add8") \
TEST_TERMINATE (ternary_terminated_1,false,0,-1,-1,"add128") \
TEST_TERMINATE (ternary_terminated_2,false,0,-1,-1,"add128") \
TEST_TERMINATE (ternary_terminated_3,false,0,-1,-1,"add128") \
TEST_TERMINATE (transitive_terminated_1,false,0,-1,-1,"add128") \
TEST_TERMINATE (transitive_terminated_2,false,0,-1,-1,"add128") \
TEST_TERMINATE (transitive_terminated_3,false,0,-1,-1,"add128") \
TEST_TERMINATE (vivify_terminated_1,false,0,-1,-1,"ph11") \
TEST_TERMINATE (vivify_terminated_2,false,0,-1,-1,"ph11") \
TEST_TERMINATE (vivify_terminated_3,false,0,-1,-1,"ph11") \
TEST_TERMINATE (vivify_terminated_4,false,0,-1,-1,"ph11") \
TEST_TERMINATE (walk_terminated_1,true,-1,-1,-1,"add8") \
TEST_TERMINATE (walk_terminated_2,true,-1,-1,-1,"add8") \
TEST_TERMINATE (xors_terminated_1,true,-1,0,-1,"add8") \

#define TEST_TERMINATE(BIT,WALKINITIALLY,ELIMININIT,PROBEINIT,REPHASEINT,CNF) \
static void test_ ## BIT (void) \
{ \
  test_terminate (BIT, #BIT, WALKINITIALLY, ELIMININIT, PROBEINIT, REPHASEINT, \
                  "../test/cnf/" CNF ".cnf"); \
}

TEST_TERMINATE_BITS

#undef TEST_TERMINATE

void
tissat_schedule_terminate (void)
{
  if (!tissat_found_test_directory)
    return;
#define TEST_TERMINATE(BIT,...) \
  SCHEDULE_FUNCTION (test_ ## BIT);
  TEST_TERMINATE_BITS
#undef TEST_TERMINATE
}

// *INDENT-ON*

#else

void
tissat_schedule_terminate (void)
{
}

#endif
