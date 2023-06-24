#ifndef NOPTIONS

#include "../src/config.h"

// Manually copied from '../src/config.c'.

#define CONFIGURATIONS \
  CONFIGURATION (basic) \
  CONFIGURATION (default) \
  CONFIGURATION (plain) \
  CONFIGURATION (sat) \
  CONFIGURATION (unsat)

#include "test.h"

static void test_config_has (void) {
#define CONFIGURATION(NAME) \
  assert (kissat_has_configuration (#NAME)); \
  printf ("checked 'kissat_has_configuration (\"%s\")'\n", #NAME);
  CONFIGURATIONS
#undef CONFIGURATION
  assert (!kissat_has_configuration ("invalid"));
  printf ("checked '!kissat_has_configuration (\"invalid\")'\n");
}

static void test_config_set (void) {
#define CONFIGURATION(NAME) \
  do { \
    DECLARE_AND_INIT_SOLVER (solver); \
    assert (kissat_set_configuration (solver, #NAME)); \
    printf ("checked 'kissat_set_configuration (..., \"%s\")'\n", #NAME); \
  } while (0);
  CONFIGURATIONS
#undef CONFIGURATION
  {
    DECLARE_AND_INIT_SOLVER (solver);
    assert (!kissat_set_configuration (solver, "invalid"));
    printf ("checked '!kissat_set_configuration (..., \"invalid\")'\n");
  }
}
#endif

void tissat_schedule_config (void) {
#ifndef NOPTIONS
  SCHEDULE_FUNCTION (test_config_has);
  SCHEDULE_FUNCTION (test_config_set);
#endif
}
