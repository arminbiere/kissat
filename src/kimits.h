#ifndef _limits_h_INCLUDED
#define _limits_h_INCLUDED

#include <stdbool.h>
#include <stdint.h>

typedef struct bounds bounds;
typedef struct changes changes;
typedef struct delays delays;
typedef struct delay delay;
typedef struct effort effort;
typedef struct enabled enabled;
typedef struct limited limited;
typedef struct limits limits;
typedef struct waiting waiting;

struct bounds
{
  struct
  {
    uint64_t max_bound_completed;
    unsigned additional_clauses;
  } eliminate;
};

struct limits
{
  uint64_t conflicts;
  uint64_t decisions;
  uint64_t reports;

  struct
  {
    uint64_t ticks;
    uint64_t conflicts;
    uint64_t interval;
  } mode;

  struct
  {
    struct
    {
      uint64_t added;
      uint64_t removed;
    } variables;
    uint64_t conflicts;
  } eliminate;

  struct
  {
    uint64_t conflicts;
  } probe, reduce, rephase, restart;
};

struct limited
{
  bool conflicts;
  bool decisions;
};

struct enabled
{
  bool eliminate;
  bool focus;
  bool mode;
  bool probe;
};

struct delay
{
  unsigned count;
  unsigned current;
};

struct delays
{
  delay backbone;
  delay bumpreasons;
  delay eliminate;
  delay failed;
  delay probe;
  delay substitute;
};

struct effort
{
  uint64_t eliminate;
  uint64_t probe;
};

struct waiting
{
  struct
  {
    uint64_t reduce;
  } eliminate, probe;
};

struct kissat;

changes kissat_changes (struct kissat *);

bool kissat_changed (changes before, changes after);

void kissat_init_limits (struct kissat *);

uint64_t kissat_scale_delta (struct kissat *, const char *, uint64_t);

double kissat_quadratic (uint64_t);
double kissat_nlogpown (uint64_t, unsigned);
double kissat_sqrt (uint64_t);
double kissat_logn (uint64_t);

#define NLOGN(COUNT) kissat_nlogpown (COUNT,1)
#define NLOG2N(COUNT) kissat_nlogpown (COUNT,2)
#define NLOG3N(COUNT) kissat_nlogpown (COUNT,3)

#define SQRT(COUNT) kissat_sqrt (COUNT)

#define UPDATE_CONFLICT_LIMIT(NAME,COUNT,SCALE_COUNT_FUNCTION,SCALE_DELTA) \
do { \
  if (solver->inconsistent) \
    break; \
  const struct statistics *statistics = &solver->statistics; \
  assert (statistics->COUNT > 0); \
  struct limits *limits = &solver->limits; \
  uint64_t DELTA = GET_OPTION (NAME ## int); \
  const double SCALING = SCALE_COUNT_FUNCTION (statistics->COUNT); \
  assert (SCALING >= 1); \
  DELTA *= SCALING;  \
  const uint64_t SCALED = !(SCALE_DELTA) ? DELTA : \
    kissat_scale_delta (solver, #NAME, DELTA); \
  limits->NAME.conflicts = CONFLICTS + SCALED; \
  kissat_phase (solver, #NAME, GET (COUNT), \
		"new limit of %s after %s conflicts", \
		FORMAT_COUNT (limits->NAME.conflicts), \
		FORMAT_COUNT (SCALED)); \
} while (0)

#include <inttypes.h>

#define SET_EFFORT_LIMIT(LIMIT,NAME,START,ADDITIONAL) \
  uint64_t LIMIT; \
  do { \
    const uint64_t OLD_LIMIT = solver->statistics.START; \
    const uint64_t TICKS = solver->statistics.search_ticks; \
    const uint64_t LAST = \
      solver->probing ? solver->last.probe : solver->last.eliminate; \
    uint64_t REFERENCE = TICKS - LAST; \
    const uint64_t MINEFFORT = 1e3 * GET_OPTION (mineffort); \
    if (REFERENCE < MINEFFORT) \
      { \
	REFERENCE = MINEFFORT; \
	kissat_extremely_verbose (solver, \
	  #NAME " effort reference %s set to 'mineffort'", \
	  FORMAT_COUNT (REFERENCE)); \
      } \
    else \
      { \
	kissat_extremely_verbose (solver, \
	  #NAME " effort reference %s = %s - %s 'search_ticks'", \
	  FORMAT_COUNT (REFERENCE), \
	    FORMAT_COUNT (TICKS), FORMAT_COUNT (LAST)); \
      } \
    const uint64_t ADJUSTMENT = (ADDITIONAL); \
    const double EFFORT = (double) GET_OPTION (NAME ## effort) * 1e-3; \
    const uint64_t PRODUCT = EFFORT * REFERENCE; \
    uint64_t DELTA = PRODUCT + ADJUSTMENT; \
    \
    kissat_extremely_verbose (solver, \
      #NAME \
      " effort delta %s = %s + %s = %g * %s + %s '" \
      #START "'", \
      FORMAT_COUNT (DELTA), \
      FORMAT_COUNT (PRODUCT), FORMAT_COUNT (ADJUSTMENT), \
      EFFORT, FORMAT_COUNT (REFERENCE), FORMAT_COUNT (ADJUSTMENT)); \
      \
    const uint64_t NEW_LIMIT = OLD_LIMIT + DELTA; \
    kissat_very_verbose (solver, \
      #NAME " effort limit %s = %s + %s '" #START "'", \
      FORMAT_COUNT (NEW_LIMIT), \
	FORMAT_COUNT (OLD_LIMIT), FORMAT_COUNT (DELTA)); \
    \
    LIMIT = NEW_LIMIT; \
    \
  } while (0)

#endif
