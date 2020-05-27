#ifdef COVERAGE

#include "test.h"

#include "../src/parse.h"

static void
test_terminate (int bit,
		bool walkinitiially,
		int probeinit,
		int eliminateinit, int rephaseint, const char *cnf)
{
#ifdef NOPTIONS
  if (walkinitiially)
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
  if (walkinitiially)
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
  tissat_verbose ("solving '%s' forcing 'TERMINATED (%d)'", cnf, bit);
  assert (0 <= bit), assert (bit < 32);
  solver->terminate = (1u << bit);
  int res = kissat_solve (solver);
  if (res)
    FATAL ("solver returned '%d' but expected '0'", res);
  else
    tissat_verbose ("solver returned '0' as expected");
  kissat_release (solver);
}

// *INDENT-OFF*

#if 1
#define TEST_TERMINATE_BITS \
TEST_TERMINATE (0,false,-1,-1,10,"hard") \
TEST_TERMINATE (1,false,-1,-1,10,"hard") \
TEST_TERMINATE (2,false,-1,-1,10,"hard") \
TEST_TERMINATE (3,false,-1,-1,10,"hard") \
TEST_TERMINATE (4,false,-1,0,-1,"add8") \
TEST_TERMINATE (5,false,-1,0,-1,"add8") \
TEST_TERMINATE (6,false,-1,0,-1,"add8") \
TEST_TERMINATE (7,false,0,-1,-1,"hard") \
TEST_TERMINATE (8,false,0,-1,-1,"hard") \
TEST_TERMINATE (9,false,0,0,-1,"add8") \
TEST_TERMINATE (10,false,-1,-1,10,"hard") \
TEST_TERMINATE (11,false,0,-1,-1,"hard") \
TEST_TERMINATE (12,false,0,-1,-1,"hard") \
TEST_TERMINATE (13,false,0,-1,-1,"add8") \
TEST_TERMINATE (14,false,0,-1,-1,"add8") \
TEST_TERMINATE (15,false,0,-1,-1,"hard") \
TEST_TERMINATE (16,false,0,-1,-1,"hard") \
TEST_TERMINATE (17,false,0,-1,-1,"hard") \
TEST_TERMINATE (18,false,0,-1,-1,"hard") \
TEST_TERMINATE (19,false,0,-1,-1,"add8") \
TEST_TERMINATE (20,false,0,-1,-1,"add8") \
TEST_TERMINATE (21,false,0,-1,-1,"add8") \
TEST_TERMINATE (22,false,0,-1,-1,"add8") \
TEST_TERMINATE (23,true,-1,-1,-1,"hard") \
TEST_TERMINATE (24,true,-1,-1,-1,"hard") \
TEST_TERMINATE (25,false,-1,-1,-1,"hard") \
TEST_TERMINATE (26,false,-1,0,-1,"add8")
#else
#define TEST_TERMINATE_BITS \
TEST_TERMINATE (14,false,0,-1,-1,"add8")
#endif

#define TEST_TERMINATE(BIT,WALKINITIALLY,ELIMININIT,PROBEINIT,REPHASEINT,CNF) \
static void test_terminate_bit_ ## BIT (void) \
{ \
  test_terminate (BIT, WALKINITIALLY, ELIMININIT, PROBEINIT, REPHASEINT, \
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
  SCHEDULE_FUNCTION (test_terminate_bit_ ## BIT);
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
