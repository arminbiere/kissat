#if !defined(QUIET) || !defined(NDEBUG)
#include "internal.h"
#endif

#ifndef QUIET

#include "resources.h"
#include "utilities.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

// *INDENT-OFF*

void
kissat_statistics_print (kissat * solver, bool verbose)
{
#ifndef QUIET
  statistics *statistics = &solver->statistics;

  const double time = kissat_process_time ();
  size_t variables = SIZE_STACK (solver->import);
#ifndef NMETRICS
  const double rss = kissat_maximum_resident_set_size ();
#endif

/*------------------------------------------------------------------------*/

#define RELATIVE(FIRST,SECOND) \
  kissat_average (statistics->FIRST, statistics->SECOND)

/*------------------------------------------------------------------------*/

#define CONF_INT(NAME) \
  RELATIVE (conflicts, NAME)

#define NO_SECONDARY(NAME) \
  0

/*------------------------------------------------------------------------*/

#define PER_BIN_RESOLVED(NAME) \
  RELATIVE (NAME, hyper_binary_resolved)

#define PER_CLS_ADDED(NAME) \
  RELATIVE (NAME, clauses_added)

#define PER_CLS_LEARNED(NAME) \
  RELATIVE (NAME, learned)

#define PER_CONFLICT(NAME) \
  RELATIVE (NAME, conflicts)

#ifdef NSTATISTICS
#define PER_FLIPPED(NAME) \
  -1
#else
#define PER_FLIPPED(NAME) \
  RELATIVE (NAME, flipped)
#endif

#define PER_FIXED(NAME) \
  RELATIVE (NAME, units)

#define PER_PROPAGATION(NAME) \
  RELATIVE (NAME, propagations)

#define PER_REUSED_TRAIL(NAME) \
  RELATIVE (NAME, restarts_reused_trails)

#define PER_SECOND(NAME) \
  kissat_average (statistics->NAME, time)

#ifdef NMETRICS
#define PER_TRN_RESOLVED(NAME) \
  -1
#else
#define PER_TRN_RESOLVED(NAME) \
  RELATIVE (NAME, hyper_ternary_resolved)
#endif

#define PER_VARIABLE(NAME) \
  kissat_average (statistics->NAME, variables)

#define PER_VIVIFICATION_CHECK(NAME) \
  RELATIVE (NAME, vivification_checks)

#ifdef NSTATISTICS
#define PER_WALKS(NAME) \
  -1
#else
#define PER_WALKS(NAME) \
  RELATIVE (NAME, walks)
#endif

/*------------------------------------------------------------------------*/

#define PERCENT(FIRST,SECOND) \
  kissat_percent (statistics->FIRST, statistics->SECOND)

/*------------------------------------------------------------------------*/

#define PCNT_ARENA_RESIZED(NAME) \
  PERCENT (NAME, arena_resized)

#define PCNT_BIN_RESOLVED(NAME) \
  PERCENT (NAME, hyper_binary_resolved)

#define PCNT_CLS_ADDED(NAME) \
  PERCENT (NAME, clauses_added)

#define PCNT_COLLECTIONS(NAME) \
  PERCENT (NAME, garbage_collections)

#define PCNT_CONFLICTS(NAME) \
  PERCENT (NAME, conflicts)

#define PCNT_DECISIONS(NAME) \
  PERCENT (NAME, decisions)

#define PCNT_DEFRAGS(NAME) \
  PERCENT (NAME, defragmentations)

#define PCNT_ELIMINATED(NAME) \
  PERCENT (NAME, eliminated)

#define PCNT_TRN_ADDED(NAME) \
  PERCENT (NAME, hyper_ternaries_added)

#define PCNT_TRN_RESOLVED(NAME) \
  PERCENT (NAME, hyper_ternary_resolved)

#ifdef NSTATISTICS
#define PCNT_TICKS(NAME) \
  -1
#else
#define PCNT_TICKS(NAME) \
  PERCENT (NAME, ticks)
#endif

#define PCNT_LITERALS_DEDUCED(NAME) \
  PERCENT (NAME, literals_deduced)

#define PCNT_PROPS(NAME) \
  PERCENT (NAME, propagations)

#define PCNT_REDUCTIONS(NAME) \
  PERCENT (NAME, reductions)

#define PCNT_REDUNDANT_CLAUSES(NAME) \
  PERCENT (NAME, clauses_redundant)

#define PCNT_RESIDENT_SET(NAME) \
  kissat_percent (statistics->NAME, rss)

#define PCNT_RESTARTS(NAME) \
  PERCENT (NAME, restarts)

#define PCNT_SEARCHES(NAME) \
  PERCENT (NAME, searches)

#define PCNT_STR(NAME) \
  PERCENT (NAME, strengthened)

#define PCNT_SUB(NAME) \
  PERCENT (NAME, subsumed)

#define PCNT_SUBSUMPTION_CHECK(NAME) \
  PERCENT (NAME, subsumption_checks)

#define PCNT_VARIABLES(NAME) \
  kissat_percent (statistics->NAME, variables)

#define PCNT_VIVIFICATION_CHECK(NAME) \
  PERCENT (NAME, vivification_checks)

#define PCNT_VIVIFIED(NAME) \
  PERCENT (NAME, vivified)

#define PCNT_VIVIFY_PROBES(NAME) \
  PERCENT (NAME, vivify_probes)

#define PCNT_VIVIFY_STR(NAME) \
  PERCENT (NAME, vivify_strengthened)

#define PCNT_VIVIFY_SUB(NAME) \
  PERCENT (NAME, vivify_subsumed)

#define PCNT_WALKS(NAME) \
  PERCENT (NAME, walks)

#define COUNTER(NAME,VERBOSE,OTHER,UNITS,TYPE) \
  if (verbose || !VERBOSE || (VERBOSE == 1 && statistics->NAME)) \
    PRINT_STAT (#NAME, statistics->NAME, OTHER(NAME), UNITS, TYPE);
#define IGNORE(...)

    STATISTICS

#undef COUNTER
#undef METRIC
#undef STATISTIC

    fflush (stdout);

#else
  (void) verbose;
  (void) statistics;
#endif

}

// *INDENT-ON*

#elif defined(NDEBUG)
int kissat_statistics_dummy_to_avoid_warning;
#endif

/*------------------------------------------------------------------------*/

#ifndef NDEBUG

void
kissat_check_statistics (kissat * solver)
{
  if (solver->inconsistent)
    return;

  size_t redundant = 0;
  size_t irredundant = 0;
  size_t hyper_ternaries = 0;
  size_t arena_garbage = 0;

  for (all_clauses (c))
    {
      if (c->garbage)
	{
	  arena_garbage += kissat_actual_bytes_of_clause (c);
	  continue;
	}
      if (c->hyper)
	hyper_ternaries++;
      if (c->redundant)
	redundant++;
      else
	irredundant++;
    }

  size_t redundant_binary_watches = 0;
  size_t irredundant_binary_watches = 0;
  size_t hyper_binaries = 0;

  if (solver->watching)
    {
      for (all_literals (lit))
	{
	  watches *watches = &WATCHES (lit);

	  for (all_binary_blocking_watches (watch, *watches))
	    {
	      if (watch.type.binary)
		{
		  if (watch.binary.redundant)
		    {
		      redundant_binary_watches++;
		      if (watch.binary.hyper)
			hyper_binaries++;
		    }
		  else
		    irredundant_binary_watches++;
		}
	    }
	}
    }
  else
    {
      for (all_literals (lit))
	{
	  watches *watches = &WATCHES (lit);

	  for (all_binary_large_watches (watch, *watches))
	    {
	      if (watch.type.binary)
		{
		  if (watch.binary.redundant)
		    {
		      redundant_binary_watches++;
		      if (watch.binary.hyper)
			hyper_binaries++;
		    }
		  else
		    irredundant_binary_watches++;
		}
	    }
	}
    }

  assert (!(redundant_binary_watches & 1));
  assert (!(irredundant_binary_watches & 1));
  assert (!(hyper_binaries & 1));

  redundant += redundant_binary_watches / 2;
  irredundant += irredundant_binary_watches / 2;
  hyper_binaries /= 2;

  statistics *statistics = &solver->statistics;
  assert (statistics->clauses_redundant == redundant);
  assert (statistics->clauses_irredundant == irredundant);
#ifndef NMETRICS
  assert (statistics->hyper_binaries == hyper_binaries);
  assert (statistics->hyper_ternaries == hyper_ternaries);
  assert (statistics->arena_garbage == arena_garbage);
#else
  (void) hyper_binaries;
  (void) hyper_ternaries;
  (void) arena_garbage;
#endif
}

#endif
