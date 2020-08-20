#include "autarky.h"
#include "backtrack.h"
#include "decide.h"
#include "internal.h"
#include "logging.h"
#include "print.h"
#include "rephase.h"
#include "report.h"
#include "terminate.h"
#include "walk.h"

#include <inttypes.h>

void
kissat_reset_rephased (kissat * solver)
{
  LOG ("reset rephase type and counter");
  solver->rephased.type = 0;
}

void
kissat_reset_target_assigned (kissat * solver)
{
  if (!solver->target_assigned)
    return;
  LOG ("resetting target assigned %u", solver->target_assigned);
  solver->target_assigned = 0;
}

void
kissat_reset_consistently_assigned (kissat * solver)
{
  if (!solver->consistently_assigned)
    return;
  LOG ("resetting consistently assigned %u", solver->consistently_assigned);
  solver->consistently_assigned = 0;
}

bool
kissat_rephasing (kissat * solver)
{
  if (!GET_OPTION (rephase))
    return false;
  return CONFLICTS > solver->limits.rephase.conflicts;
}

static char
rephase_best (kissat * solver)
{
  for (all_phases (p))
    if (p->best)
      p->saved = p->best;
  return 'B';
}

char
kissat_rephase_best (kissat * solver)
{
  return rephase_best (solver);
}

static char
rephase_flipped (kissat * solver)
{
  for (all_phases (p))
    p->saved *= -1;
  return 'F';
}

static char
rephase_original (kissat * solver)
{
  const value value = INITIAL_PHASE;
  for (all_phases (p))
    p->saved = value;
  return 'O';
}

static char
rephase_inverted (kissat * solver)
{
  const value value = -INITIAL_PHASE;
  for (all_phases (p))
    p->saved = value;
  return 'I';
}

static char
rephase_random (kissat * solver)
{
  for (all_phases (p))
    p->saved = (kissat_pick_bool (&solver->random) ? 1 : -1);
  return '#';
}

static char
rephase_walking (kissat * solver)
{
  STOP (rephase);
  char res = kissat_walk (solver);
  if (res == 'W')
    kissat_autarky (solver);
  START (rephase);
  if (!res)
    res = rephase_best (solver);
  return res;
}

static void
reset_phases (kissat * solver)
{
  kissat_clear_target_phases (solver);
  kissat_reset_target_assigned (solver);
  const uint64_t count = solver->rephased.count++;

  char type = 0;

  if (!count)
    type = rephase_original (solver);
  else if (count == 1)
    type = rephase_inverted (solver);
  else
    {
      switch ((count - 2) % 12)
	{
	default:
	case 0:
	  type = rephase_best (solver);
	  break;
	case 1:
	  type = rephase_walking (solver);
	  break;
	case 2:
	  type = rephase_original (solver);
	  break;
	case 3:
	  type = rephase_best (solver);
	  break;
	case 4:
	  type = rephase_walking (solver);
	  break;
	case 5:
	  type = rephase_inverted (solver);
	  break;
	case 6:
	  type = rephase_best (solver);
	  break;
	case 7:
	  type = rephase_walking (solver);
	  break;
	case 8:
	  type = rephase_random (solver);
	  break;
	case 9:
	  type = rephase_best (solver);
	  break;
	case 10:
	  type = rephase_walking (solver);
	  break;
	case 11:
	  type = rephase_flipped (solver);
	  break;
	}
    }
#ifndef QUIET
  const char *type_as_string = 0;
  switch (type)
    {
    case 'B':
      type_as_string = "best";
      break;
    case 'F':
      type_as_string = "flipped";
      break;
    case 'I':
      type_as_string = "inverted";
      break;
    case 'O':
      type_as_string = "original";
      break;
    case '#':
      type_as_string = "random";
      break;
    case 'W':
      type_as_string = "walking";
      break;
    }
  assert (type_as_string);
  kissat_phase (solver, "rephase", GET (rephased),
		"%s phases in %s search mode",
		type_as_string, solver->stable ? "stable" : "focused");
#endif
  LOG ("remember last rephase type '%c'", type);
  solver->rephased.type = type;
  solver->rephased.last = CONFLICTS;

  UPDATE_CONFLICT_LIMIT (rephase, rephased, LINEAR, false);
}

void
kissat_rephase (kissat * solver)
{
  kissat_backtrack_propagate_and_flush_trail (solver);
  assert (!solver->inconsistent);
  kissat_autarky (solver);
  if (TERMINATED (10))
    return;
  START (rephase);
  INC (rephased);
  REPORT (1, '~');
  reset_phases (solver);
  STOP (rephase);
}
