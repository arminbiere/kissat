#ifndef NOPTIONS

#include "config.h"
#include "kissat.h"
#include "options.h"

#include <stdio.h>
#include <string.h>

bool
kissat_has_configuration (const char *name)
{
  if (!strcmp (name, "default"))
    return true;
  if (!strcmp (name, "sat"))
    return true;
  if (!strcmp (name, "unsat"))
    return true;
  return false;
}

void
kissat_configuration_usage (void)
{
  const char *fmt = "  --%-24s %s\n";
  printf (fmt, "default", "default configuration");
  printf (fmt, "sat", "target satisfiable instances ('--target=2')");
  printf (fmt, "unsat", "target unsatisfiable instances ('--stable=0')");
}

void
kissat_set_configuration (kissat * solver, const char *name)
{
  if (!strcmp (name, "default"))
    return;
  if (!strcmp (name, "sat"))
    {
      kissat_set_option (solver, "target", TARGET_SAT);
      return;
    }
  if (!strcmp (name, "unsat"))
    {
      kissat_set_option (solver, "stable", STABLE_UNSAT);
      return;
    }
}

#else
int kissat_config_dummy_to_avoid_warning;
#endif
