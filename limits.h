#ifndef _limits_h_INCLUDED
#define _limits_h_INCLUDED

#include <stdbool.h>
#include <stdint.h>

typedef struct bounds bounds;
typedef struct changes changes;
typedef struct delays delays;
typedef struct delay delay;
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
    unsigned clause_size;
    unsigned occurrences;
  } eliminate;

  struct
  {
    unsigned clause_size;
    unsigned occurrences;
  } subsume;

  struct
  {
    unsigned clause_size;
  } xor;
};

struct changes
{
  struct
  {
    uint64_t added;
    uint64_t removed;
    unsigned units;
  } variables;
  struct
  {
    unsigned additional_clauses;
  } eliminate;
};

struct limits
{
  uint64_t conflicts;
  uint64_t decisions;
  uint64_t reports;

  union
  {
    uint64_t ticks;
    uint64_t conflicts;
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
  bool autarky;
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
  delay autarky;
  delay eliminate;
  delay failed;
  delay probe;
  delay substitute;
  delay ternary;
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

uint64_t kissat_scale_limit (struct kissat *,
			     const char *, uint64_t count, int base);

#define SCALE_LIMIT(COUNT,NAME) \
  kissat_scale_limit (solver, #NAME, \
                      solver->statistics.COUNT, GET_OPTION (NAME))

uint64_t kissat_logn (uint64_t);
uint64_t kissat_ndivlogn (uint64_t);
uint64_t kissat_linear (uint64_t);
uint64_t kissat_nlogn (uint64_t);
uint64_t kissat_nlognlogn (uint64_t);
uint64_t kissat_quadratic (uint64_t);

#define NDIVLOGN(COUNT) kissat_ndivlogn (COUNT)
#define LINEAR(COUNT) kissat_linear (COUNT)
#define NLOGN(COUNT) kissat_nlogn (COUNT)
#define NLOGNLOGN(COUNT) kissat_nlognlogn (COUNT)
#define QUADRATIC(COUNT) kissat_quadratic (COUNT)

#define INIT_CONFLICT_LIMIT(NAME,SCALE) \
do { \
  const uint64_t DELTA = GET_OPTION (NAME ## init); \
  const uint64_t SCALED = !(SCALE) ? DELTA : \
    kissat_scale_delta (solver, #NAME, DELTA); \
  limits->NAME.conflicts = CONFLICTS + SCALED; \
  kissat_very_verbose (solver, \
    "initial " #NAME " limit of %s conflicts", \
    FORMAT_COUNT (limits->NAME.conflicts)); \
} while (0)

#define UPDATE_CONFLICT_LIMIT(NAME,COUNT,SCALE_COUNT_FUNCTION,SCALE_DELTA) \
do { \
  if (solver->inconsistent) \
    break; \
  struct statistics *statistics = &solver->statistics; \
  struct limits *limits = &solver->limits; \
  uint64_t DELTA = GET_OPTION (NAME ## int); \
  DELTA *= SCALE_COUNT_FUNCTION (statistics->COUNT) + 1; \
  const uint64_t SCALED = !(SCALE_DELTA) ? DELTA : \
    kissat_scale_delta (solver, #NAME, DELTA); \
  limits->NAME.conflicts = CONFLICTS + SCALED; \
  kissat_phase (solver, #NAME, \
		GET (COUNT), \
		"new limit of %s after %s conflicts", \
		FORMAT_COUNT (limits->NAME.conflicts), \
		FORMAT_COUNT (SCALED)); \
} while (0)

#include <inttypes.h>

#define SET_EFFICIENCY_BOUND(BOUND,NAME,START,REFERENCE,ADDITIONAL) \
  uint64_t BOUND = solver->statistics.START; \
  do { \
    const uint64_t REFERENCE = solver->statistics.REFERENCE; \
    const uint64_t MINIMUM = GET_OPTION (NAME ## mineff); \
    const uint64_t MAXIMUM = MINIMUM * GET_OPTION (NAME ## maxeff); \
    const double EFFICIENCY = (double) GET_OPTION (NAME ## releff) / 1e3; \
    const uint64_t ADJUSTMENT = (ADDITIONAL); \
    const uint64_t PRODUCT = REFERENCE * EFFICIENCY; \
    uint64_t DELTA = PRODUCT + ADJUSTMENT; \
    if (DELTA < MINIMUM) \
      DELTA = MINIMUM; \
    if (DELTA > MAXIMUM) \
      DELTA = MAXIMUM; \
    BOUND += DELTA; \
    kissat_very_verbose (solver, \
      #NAME " efficiency limit %s delta %s = %s + %s", \
      FORMAT_COUNT (BOUND), FORMAT_COUNT (DELTA), \
      FORMAT_COUNT (PRODUCT), FORMAT_COUNT (ADJUSTMENT)); \
  } while (0)

#define RETURN_IF_DELAYED(NAME) \
do { \
  assert (!solver->inconsistent); \
  if (!GET_OPTION (NAME ## delay)) \
    break; \
  delay * DELAY = &solver->delays.NAME; \
  assert (DELAY->count <= DELAY->current); \
  if (!DELAY->count) \
    break; \
  kissat_very_verbose (solver, \
                       #NAME " delayed %u more time%s", \
		       DELAY->count, DELAY->count > 1 ? "s" : ""); \
  DELAY->count--; \
  return; \
} while (0)

#define UPDATE_DELAY(SUCCESS,NAME) \
do { \
  if (solver->inconsistent) \
    break; \
  if (!GET_OPTION (NAME ## delay)) \
    break; \
  delay * DELAY = &solver->delays.NAME; \
  unsigned MAX_DELAY = GET_OPTION (delay); \
  assert (DELAY->count <= DELAY->current); \
  if (SUCCESS) \
    { \
      if (DELAY->current) \
	{ \
	  kissat_very_verbose (solver, #NAME " delay reset"); \
	  DELAY->current = DELAY->count = 0; \
	} \
      else \
	assert (!DELAY->count); \
    } \
  else \
    { \
      if (DELAY->current < MAX_DELAY) \
	{ \
	  DELAY->current++; \
	  kissat_very_verbose (solver, \
			       #NAME " delay increased to %u", \
			       DELAY->current); \
	} \
      else \
	kissat_very_verbose (solver, \
			     "keeping " #NAME " delay at maximum %u", \
			     DELAY->current); \
      DELAY->count = DELAY->current; \
    } \
  assert (DELAY->count <= DELAY->current); \
} while (0)

#endif
