#include "allocate.h"
#include "decide.h"
#include "dense.h"
#include "inline.h"
#include "print.h"
#include "report.h"
#include "rephase.h"
#include "terminate.h"
#include "walk.h"

typedef struct tagged tagged;
typedef struct counter counter;
typedef struct walker walker;

#define LD_MAX_WALK_REF 31
#define MAX_WALK_REF ((1u << LD_MAX_WALK_REF) - 1)

struct tagged
{
  unsigned ref:LD_MAX_WALK_REF;
  bool binary:1;
};

static inline tagged
make_tagged (bool binary, unsigned ref)
{
  assert (ref <= MAX_WALK_REF);
  tagged res = {.binary = binary,.ref = ref };
  return res;
}

struct counter
{
  unsigned count;
  unsigned pos;
};

// *INDENT-OFF*
typedef STACK (double) doubles;
// *INDENT-ON*

#define INVALID_BEST UINT_MAX

struct walker
{
  kissat *solver;
  generator random;
  counter *counters;
  litpairs *binaries;
  value *saved;
  unsigned clauses;
  unsigned offset;
  unsigned minimum;
  unsigned current;
  unsigned initial;
  tagged *refs;
  double *table;
  double epsilon;
  unsigned exponents;
  doubles scores;
  unsigneds unsat;
  unsigneds trail;
  unsigned best;
  uint64_t limit;
#ifndef QUIET
  uint64_t start;
  uint64_t flipped;
  struct
  {
    uint64_t flipped;
    unsigned minimum;
  } report;
#endif
};

static const unsigned *
dereference_literals (kissat * solver, walker * walker,
		      unsigned counter_ref, unsigned *size_ptr)
{
  assert (counter_ref < walker->clauses);
  tagged tagged = walker->refs[counter_ref];
  const unsigned *lits;
  if (tagged.binary)
    {
      const unsigned binary_ref = tagged.ref;
      lits = PEEK_STACK (*walker->binaries, binary_ref).lits;
      *size_ptr = 2;
    }
  else
    {
      const reference clause_ref = tagged.ref;
      clause *c = kissat_dereference_clause (solver, clause_ref);
      *size_ptr = c->size;
      lits = c->lits;
    }
  return lits;
}

static void
push_unsat (kissat * solver, walker * walker,
	    counter * counters, unsigned counter_ref)
{
  assert (counter_ref < walker->clauses);
  counter *counter = counters + counter_ref;
  assert (SIZE_STACK (walker->unsat) <= UINT_MAX);
  counter->pos = SIZE_STACK (walker->unsat);
  PUSH_STACK (walker->unsat, counter_ref);
#ifdef LOGGING
  unsigned size;
  const unsigned *lits = dereference_literals (solver, walker,
					       counter_ref, &size);
  LOGLITS (size, lits, "pushed unsatisfied[%u]", counter->pos);
#endif
}

static bool
pop_unsat (kissat * solver, walker * walker,
	   counter * counters, unsigned counter_ref, unsigned pos)
{
  assert (walker->current);
  assert (counter_ref < walker->clauses);
  assert (counters[counter_ref].pos == pos);
  assert (walker->current == SIZE_STACK (walker->unsat));
  const unsigned other_counter_ref = POP_STACK (walker->unsat);
  walker->current--;
  bool res = false;
  if (counter_ref != other_counter_ref)
    {
      assert (other_counter_ref < walker->clauses);
      counter *other_counter = counters + other_counter_ref;
      assert (other_counter->pos == walker->current);
      assert (pos < other_counter->pos);
      other_counter->pos = pos;
      POKE_STACK (walker->unsat, pos, other_counter_ref);
      res = true;
    }
#ifdef LOGGING
  unsigned size;
  const unsigned *lits = dereference_literals (solver, walker,
					       counter_ref, &size);
  LOGLITS (size, lits, "popped unsatisfied[%u]", pos);
#else
  (void) solver;
#endif
  return res;
}

static void
init_score_table (walker * walker)
{
  kissat *solver = walker->solver;

  const double cb = 2.0;
  const double base = 1 / cb;

  double next;
  unsigned exponents = 0;
  for (next = 1; next; next *= base)
    exponents++;

  walker->table = kissat_malloc (solver, exponents * sizeof (double));

  unsigned i = 0;
  double epsilon;
  for (epsilon = next = 1; next; next = epsilon * base)
    walker->table[i++] = epsilon = next;

  assert (i == exponents);
  walker->exponents = exponents;
  walker->epsilon = epsilon;

  kissat_phase (solver, "walk", GET (walks),
		"CB %.2f with inverse %.2f as base", cb, base);
  kissat_phase (solver, "walk", GET (walks),
		"table size %u and epsilon %g", exponents, epsilon);
}

static unsigned
currently_unsatified (walker * walker)
{
  return SIZE_STACK (walker->unsat);
}

static void
import_decision_phases (walker * walker)
{
  kissat *solver = walker->solver;
  const phase *phases = solver->phases;
  const value initial_phase = INITIAL_PHASE;
  const flags *flags = solver->flags;
  const bool stable = solver->stable;
  value *values = solver->values;
  for (all_variables (idx))
    {
      if (!flags[idx].active)
	continue;
      const phase *p = phases + idx;
      value value = 0;
      if (stable)
	value = p->target;
      if (!value)
	value = p->saved;
      if (!value)
	value = initial_phase;
      assert (value);
      const unsigned lit = LIT (idx);
      const unsigned not_lit = NOT (lit);
      values[lit] = value;
      values[not_lit] = -value;
      LOG ("copied variable %u decision phase %d", idx, (int) value);
    }
  kissat_phase (solver, "walk", GET (walks), "copied decision phases");
}

static unsigned
connect_binary_counters (walker * walker)
{
  kissat *solver = walker->solver;
  value *values = solver->values;
  tagged *refs = walker->refs;
  watches *all_watches = solver->watches;
  counter *counters = walker->counters;

  assert (SIZE_STACK (*walker->binaries) <= UINT_MAX);
  const unsigned size = SIZE_STACK (*walker->binaries);
  litpair *binaries = BEGIN_STACK (*walker->binaries);
  unsigned unsat = 0, counter_ref = 0;

  for (unsigned binary_ref = 0; binary_ref < size; binary_ref++)
    {
      const litpair *litpair = binaries + binary_ref;
      const unsigned first = litpair->lits[0];
      const unsigned second = litpair->lits[1];
      assert (first < LITS), assert (second < LITS);
      const value first_value = values[first];
      const value second_value = values[second];
      if (!first_value || !second_value)
	continue;
      assert (counter_ref < walker->clauses);
      refs[counter_ref] = make_tagged (true, binary_ref);
      watches *first_watches = all_watches + first;
      watches *second_watches = all_watches + second;
      kissat_push_large_watch (solver, first_watches, counter_ref);
      kissat_push_large_watch (solver, second_watches, counter_ref);
      const unsigned count = (first_value > 0) + (second_value > 0);
      counter *counter = counters + counter_ref;
      counter->count = count;
      if (!count)
	{
	  push_unsat (solver, walker, counters, counter_ref);
	  unsat++;
	}
      counter_ref++;
    }
  kissat_phase (solver, "walk", GET (walks),
		"initially %u unsatisfied binary clauses %.0f%% out of %u",
		unsat, kissat_percent (unsat, counter_ref), counter_ref);
#ifdef QUIET
  (void) unsat;
#endif
  return counter_ref;
}

static void
connect_large_counters (walker * walker, unsigned counter_ref)
{
  kissat *solver = walker->solver;
  assert (!solver->level);
  const value *saved = walker->saved;
  const value *values = solver->values;
  const word *arena = BEGIN_STACK (solver->arena);
  counter *counters = walker->counters;
  tagged *refs = walker->refs;

  unsigned unsat = 0;
  unsigned large = 0;

  clause *last_irredundant = kissat_last_irredundant_clause (solver);

  for (all_clauses (c))
    {
      if (last_irredundant && c > last_irredundant)
	break;
      if (c->garbage)
	continue;
      if (c->redundant)
	continue;
      for (all_literals_in_clause (lit, c))
	{
	  const value value = saved[lit];
	  if (value <= 0)
	    continue;
	  LOGCLS (c, "%s satisfied", LOGLIT (lit));
	  kissat_mark_clause_as_garbage (solver, c);
	  break;
	}
      if (c->garbage)
	continue;
      large++;
      assert (kissat_clause_in_arena (solver, c));
      reference clause_ref = (word *) c - arena;
      assert (clause_ref <= MAX_WALK_REF);
      assert (counter_ref < walker->clauses);
      refs[counter_ref] = make_tagged (false, clause_ref);
      unsigned count = 0;
      for (all_literals_in_clause (lit, c))
	{
	  const value value = values[lit];
	  if (!value)
	    continue;
	  watches *watches = &WATCHES (lit);
	  kissat_push_large_watch (solver, watches, counter_ref);
	  if (value < 0)
	    continue;
	  count++;
	}
      counter *counter = walker->counters + counter_ref;
      counter->count = count;
      if (!count)
	{
	  push_unsat (solver, walker, counters, counter_ref);
	  unsat++;
	}
      counter_ref++;
    }
  kissat_phase (solver, "walk", GET (walks),
		"initially %u unsatisfied large clauses %.0f%% out of %u",
		unsat, kissat_percent (unsat, large), large);
#ifdef QUIET
  (void) large;
  (void) unsat;
#endif
}

#ifndef QUIET

static void
report_initial_minimum (kissat * solver, walker * walker)
{
  walker->report.minimum = walker->minimum;
  kissat_very_verbose (solver, "initial minimum of %u unsatisfied clauses",
		       walker->minimum);
}

static void
report_minimum (const char *type, kissat * solver, walker * walker)
{
  assert (walker->minimum <= walker->report.minimum);
  kissat_very_verbose (solver,
		       "%s minimum of %u unsatisfied clauses after %"
		       PRIu64 " flipped literals", type,
		       walker->minimum, walker->flipped);
  walker->report.minimum = walker->minimum;
}
#else
#define report_initial_minimum(...) do { } while (0)
#define report_minimum(...) do { } while (0)
#endif

static void
init_walker (kissat * solver, walker * walker, litpairs * binaries)
{
  assert (IRREDUNDANT_CLAUSES <= MAX_WALK_REF);
  const unsigned clauses = IRREDUNDANT_CLAUSES;

  memset (walker, 0, sizeof *walker);

  walker->solver = solver;
  walker->clauses = clauses;
  walker->binaries = binaries;
  walker->random = solver->random;

  walker->saved = solver->values;
  solver->values = kissat_calloc (solver, LITS, 1);

  import_decision_phases (walker);

  walker->counters = kissat_malloc (solver, clauses * sizeof (counter));
  walker->refs = kissat_malloc (solver, clauses * sizeof (tagged));

  const unsigned counter_ref = connect_binary_counters (walker);
  connect_large_counters (walker, counter_ref);

  walker->current = walker->initial = currently_unsatified (walker);

  kissat_phase (solver, "walk", GET (walks),
		"initially %u unsatisfied irredundant clauses %.0f%% "
		"out of %" PRIu64, walker->initial,
		kissat_percent (walker->initial, IRREDUNDANT_CLAUSES),
		IRREDUNDANT_CLAUSES);

  walker->minimum = walker->current;
  init_score_table (walker);

  report_initial_minimum (solver, walker);
}

static void
init_walker_limit (kissat * solver, walker * walker)
{
  SET_EFFICIENCY_BOUND (limit, walk, walk_steps, search_ticks, 2 * CLAUSES);
  walker->limit = limit;

#ifndef QUIET
  walker->start = solver->statistics.walk_steps;
  walker->flipped = 0;
  walker->report.minimum = UINT_MAX;
  walker->report.flipped = 0;;
#endif
}

static void
release_walker (walker * walker)
{
  kissat *solver = walker->solver;
  kissat_dealloc (solver, walker->table, walker->exponents, sizeof (double));
  unsigned clauses = walker->clauses;
  kissat_dealloc (solver, walker->refs, clauses, sizeof (tagged));
  kissat_dealloc (solver, walker->counters, clauses, sizeof (counter));
  RELEASE_STACK (walker->unsat);
  RELEASE_STACK (walker->scores);
  RELEASE_STACK (walker->trail);
  kissat_free (solver, solver->values, LITS);
  RELEASE_STACK (walker->unsat);
  solver->values = walker->saved;
}

static unsigned
break_value (kissat * solver, walker * walker, value * values, unsigned lit)
{
  assert (values[lit] < 0);
  const unsigned not_lit = NOT (lit);
  watches *watches = &WATCHES (not_lit);
  unsigned steps = 0;
  unsigned res = 0;
  for (all_binary_large_watches (watch, *watches))
    {
      steps++;
      assert (!watch.type.binary);
      reference counter_ref = watch.large.ref;
      assert (counter_ref < walker->clauses);
      counter *counter = walker->counters + counter_ref;
      if (counter->count == 1)
	res++;
    }
  ADD (walk_steps, steps);
#ifdef NDEBUG
  (void) values;
#endif
  return res;
}

static double
scale_score (walker * walker, unsigned breaks)
{
  if (breaks < walker->exponents)
    return walker->table[breaks];
  else
    return walker->epsilon;
}

static unsigned
pick_literal (kissat * solver, walker * walker)
{
  assert (walker->current == SIZE_STACK (walker->unsat));
  const unsigned pos =
    kissat_next_random32 (&walker->random) % walker->current;
  const unsigned counter_ref = PEEK_STACK (walker->unsat, pos);
  unsigned size;
  const unsigned *lits =
    dereference_literals (solver, walker, counter_ref, &size);

  assert (EMPTY_STACK (walker->scores));

  value *values = solver->values;

  double sum = 0;
  unsigned picked_lit = INVALID_LIT;

  const unsigned *end_of_lits = lits + size;
  for (const unsigned *p = lits; p != end_of_lits; p++)
    {
      const unsigned lit = *p;
      if (!values[lit])
	continue;
      picked_lit = lit;
      const unsigned breaks = break_value (solver, walker, values, lit);
      const double score = scale_score (walker, breaks);
      assert (score > 0);
      LOG ("literal %s breaks %u score %g", LOGLIT (lit), breaks, score);
      PUSH_STACK (walker->scores, score);
      sum += score;
    }
  assert (picked_lit != INVALID_LIT);
  assert (0 < sum);

  const double random = kissat_pick_double (&walker->random);
  assert (0 <= random), assert (random < 1);

  const double threshold = sum * random;
  LOG ("score sum %g and random threshold %g", sum, threshold);

  // assert (threshold < sum); // NOT TRUE!!!!

  double *scores = BEGIN_STACK (walker->scores);
#ifdef LOGGING
  double picked_score = 0;
#endif

  sum = 0;

  for (const unsigned *p = lits; p != end_of_lits; p++)
    {
      const unsigned lit = *p;
      if (!values[lit])
	continue;
      const double score = *scores++;
      sum += score;
      if (threshold < sum)
	{
	  picked_lit = lit;
#ifdef LOGGING
	  picked_score = score;
#endif
	  break;
	}
    }
  assert (picked_lit != INVALID_LIT);
  LOG ("picked literal %s with score %g", LOGLIT (picked_lit), picked_score);

  CLEAR_STACK (walker->scores);

  return picked_lit;
}

static void
break_clauses (kissat * solver, walker * walker,
	       const value * values, unsigned flipped)
{
#ifdef LOGGING
  unsigned broken = 0;
#endif
  const unsigned not_flipped = NOT (flipped);
  assert (values[not_flipped] < 0);
  LOG ("breaking one-satisfied clauses containing negated flipped literal %s",
       LOGLIT (not_flipped));
  watches *watches = &WATCHES (not_flipped);
  counter *counters = walker->counters;
  unsigned steps = 0;
  for (all_binary_large_watches (watch, *watches))
    {
      steps++;
      assert (!watch.type.binary);
      const unsigned counter_ref = watch.large.ref;
      assert (counter_ref < walker->clauses);
      counter *counter = counters + counter_ref;
      assert (counter->count);
      if (--counter->count)
	continue;
      push_unsat (solver, walker, counters, counter_ref);
#ifdef LOGGING
      broken++;
#endif
    }
  LOG ("broken %u one-satisfied clauses containing "
       "negated flipped literal %s", broken, LOGLIT (not_flipped));
  ADD (walk_steps, steps);
#ifdef NDEBUG
  (void) values;
#endif
}

static void
make_clauses (kissat * solver, walker * walker,
	      const value * values, unsigned flipped)
{
  assert (values[flipped] > 0);
  LOG ("making unsatisfied clauses containing flipped literal %s",
       LOGLIT (flipped));
  watches *watches = &WATCHES (flipped);
  counter *counters = walker->counters;
  unsigned steps = 0;
#ifdef LOGGING
  unsigned made = 0;
#endif
  for (all_binary_large_watches (watch, *watches))
    {
      steps++;
      assert (!watch.type.binary);
      const unsigned counter_ref = watch.large.ref;
      assert (counter_ref < walker->clauses);
      counter *counter = counters + counter_ref;
      assert (counter->count < UINT_MAX);
      if (counter->count++)
	continue;
      if (pop_unsat (solver, walker, counters, counter_ref, counter->pos))
	steps++;
#ifdef LOGGING
      made++;
#endif
    }
  LOG ("made %u unsatisfied clauses containing flipped literal %s",
       made, LOGLIT (flipped));
  ADD (walk_steps, steps);
#ifdef NDEBUG
  (void) values;
#endif
}

static void
save_all_values (kissat * solver, walker * walker)
{
  assert (EMPTY_STACK (walker->trail));
  assert (walker->best == INVALID_BEST);
  LOG ("copying all values as saved phases since trail is invalid");
  const value *values = solver->values;
  phase *phases = solver->phases;
  for (all_variables (idx))
    {
      const unsigned lit = LIT (idx);
      const value value = values[lit];
      if (value)
	phases[idx].saved = value;
    }
  LOG ("reset best trail position to 0");
  walker->best = 0;
}

static void
save_walker_trail (kissat * solver, walker * walker, bool keep)
{
#if defined(LOGGING) || !defined(NDEBUG)
  assert (walker->best != INVALID_BEST);
  assert (SIZE_STACK (walker->trail) <= UINT_MAX);
  const unsigned size_trail = SIZE_STACK (walker->trail);
  assert (walker->best <= size_trail);
  const unsigned kept = size_trail - walker->best;
  LOG ("saving %u values of flipped literals on trail of size %u",
       walker->best, size_trail);
#endif
  unsigned *begin = BEGIN_STACK (walker->trail);
  const unsigned *best = begin + walker->best;
  const value *values = solver->values;
  phase *phases = solver->phases;
  for (const unsigned *p = begin; p != best; p++)
    {
      const unsigned lit = *p;
      value value = values[lit];
      assert (value);
      if (NEGATED (lit))
	value = -value;
      const unsigned idx = IDX (lit);
      phases[idx].saved = value;
    }
  if (!keep)
    {
      LOG ("no need to shift and keep remaining %u literals", kept);
      return;
    }
  LOG ("flushed %u literals %.0f%% from trail",
       walker->best, kissat_percent (walker->best, size_trail));
  const unsigned *end = END_STACK (walker->trail);
  unsigned *q = begin;
  for (const unsigned *p = best; p != end; p++)
    *q++ = *p;
  assert ((size_t) (end - q) == walker->best);
  assert ((size_t) (q - begin) == kept);
  SET_END_OF_STACK (walker->trail, q);
  LOG ("keeping %u literals %.0f%% on trail",
       kept, kissat_percent (kept, size_trail));
  LOG ("reset best trail position to 0");
  walker->best = 0;
}

static void
push_flipped (kissat * solver, walker * walker, unsigned flipped)
{
  if (walker->best == INVALID_BEST)
    {
      LOG ("not pushing flipped %s to already invalid trail",
	   LOGLIT (flipped));
      assert (EMPTY_STACK (walker->trail));
    }
  else
    {
      assert (SIZE_STACK (walker->trail) <= UINT_MAX);
      const unsigned size_trail = SIZE_STACK (walker->trail);
      assert (walker->best <= size_trail);
      const unsigned limit = VARS / 4 + 1;
      assert (limit < INVALID_BEST);
      if (size_trail < limit)
	{
	  PUSH_STACK (walker->trail, flipped);
	  LOG ("pushed flipped %s to trail which now has size %u",
	       LOGLIT (flipped), size_trail + 1);
	}
      else if (walker->best)
	{
	  LOG ("trail reached limit %u but has best position %u",
	       limit, walker->best);
	  save_walker_trail (solver, walker, true);
	  PUSH_STACK (walker->trail, flipped);
	  assert (SIZE_STACK (walker->trail) <= UINT_MAX);
	  LOG ("pushed flipped %s to trail which now has size %u",
	       LOGLIT (flipped), SIZE_STACK (walker->trail));
	}
      else
	{
	  LOG ("trail reached limit %u without best position", limit);
	  CLEAR_STACK (walker->trail);
	  LOG ("not pushing %s to invalidated trail", LOGLIT (flipped));
	  walker->best = INVALID_BEST;
	  LOG ("best trail position becomes invalid");
	}
    }
}

static void
flip_literal (kissat * solver, walker * walker, unsigned flip)
{
  LOG ("flipping literal %s", LOGLIT (flip));
  value *values = solver->values;
  const value value = values[flip];
  assert (value < 0);
  values[flip] = -value;
  values[NOT (flip)] = value;
  make_clauses (solver, walker, values, flip);
  break_clauses (solver, walker, values, flip);
  walker->current = currently_unsatified (walker);
}

static void
update_best (kissat * solver, walker * walker)
{
  assert (walker->current < walker->minimum);
  walker->minimum = walker->current;
#ifndef QUIET
  int verbosity = kissat_verbosity (solver);
  bool report = (verbosity > 2);
  if (verbosity == 2)
    {
      if (walker->flipped / 2 >= walker->report.flipped)
	report = true;
      else if (walker->minimum < 5 ||
	       walker->report.minimum == UINT_MAX ||
	       walker->minimum <= walker->report.minimum / 2)
	report = true;
      if (report)
	{
	  walker->report.minimum = walker->minimum;
	  walker->report.flipped = walker->flipped;
	}
    }
  if (report)
    report_minimum ("new", solver, walker);
#endif
  if (walker->best == INVALID_BEST)
    save_all_values (solver, walker);
  else
    {
      assert (SIZE_STACK (walker->trail) < INVALID_BEST);
      walker->best = SIZE_STACK (walker->trail);
      LOG ("new best trail position %u", walker->best);
    }
}

static void
local_search_step (kissat * solver, walker * walker)
{
  assert (walker->current);
  INC (flipped);
#ifndef QUIET
  assert (walker->flipped < UINT64_MAX);
  walker->flipped++;
#endif
  LOG ("starting local search flip %" PRIu64 " with %u unsatisfied clauses",
       GET (flipped), walker->current);
  unsigned lit = pick_literal (solver, walker);
  flip_literal (solver, walker, lit);
  push_flipped (solver, walker, lit);
  if (walker->current < walker->minimum)
    update_best (solver, walker);
  LOG ("ending local search step %" PRIu64 " with %u unsatisfied clauses",
       GET (flipped), walker->current);
}

static void
local_search_round (walker * walker, unsigned round)
{
  kissat *solver = walker->solver;
  kissat_very_verbose (solver,
		       "round %u starts with %u unsatisfied clauses",
		       round, walker->current);
  init_walker_limit (solver, walker);
#ifndef QUIET
  const unsigned before = walker->minimum;
#endif
  statistics *statistics = &solver->statistics;
  while (walker->minimum && walker->limit > statistics->walk_steps)
    {
      if (TERMINATED (23))
	break;
      local_search_step (solver, walker);
    }
#ifndef QUIET
  report_minimum ("last", solver, walker);
  assert (statistics->walk_steps >= walker->start);
  const uint64_t steps = statistics->walk_steps - walker->start;
  // *INDENT-OFF*
  kissat_very_verbose (solver,
    "round %u ends with %u unsatisfied clauses",
    round, walker->current);
  kissat_very_verbose (solver,
    "flipping %" PRIu64 " literals took %" PRIu64 " steps (%.2f per flipped)",
    walker->flipped, steps, kissat_average (steps, walker->flipped));
  // *INDENT-ON*
  const unsigned after = walker->minimum;
  kissat_phase (solver, "walk", GET (walks),
		"round %u ends with %s minimum %u after %" PRIu64 " flips",
		round, after < before ? "new" : "unchanged", after,
		walker->flipped);
#else
  (void) round;
#endif
}

static char
save_final_minimum (walker * walker)
{
  kissat *solver = walker->solver;

  assert (walker->minimum <= walker->initial);
  if (walker->minimum == walker->initial)
    {
      kissat_phase (solver, "walk", GET (walks),
		    "no improvement thus keeping saved phases");
      return 0;
    }

  if (!walker->best || walker->best == INVALID_BEST)
    LOG ("minimum already saved");
  else
    save_walker_trail (solver, walker, false);

  INC (walk_improved);
  kissat_phase (solver, "walk", GET (walks),
		"improved assignment to %u unsatisfied clauses",
		walker->minimum);
  return 'W';
}

static char
walking_phase (kissat * solver)
{
  INC (walks);
  litpairs irredundant;
  litwatches redundant;
  INIT_STACK (irredundant);
  INIT_STACK (redundant);
  kissat_enter_dense_mode (solver, &irredundant, &redundant);
  walker walker;
  init_walker (solver, &walker, &irredundant);
  const unsigned max_rounds = GET_OPTION (walkrounds);
  for (unsigned round = 1; walker.minimum && round <= max_rounds; round++)
    {
      if (TERMINATED (24))
	break;
      local_search_round (&walker, round);
    }
  char res = save_final_minimum (&walker);
  release_walker (&walker);
  kissat_resume_sparse_mode (solver, false, &irredundant, &redundant);
  RELEASE_STACK (irredundant);
  RELEASE_STACK (redundant);
  return res;
}

char
kissat_walk (kissat * solver)
{
  assert (!solver->level);
  assert (!solver->inconsistent);
  assert (kissat_propagated (solver));

  reference last_irredundant = solver->last_irredundant;
  if (last_irredundant == INVALID_REF)
    last_irredundant = SIZE_STACK (solver->arena);

  if (last_irredundant > MAX_WALK_REF)
    {
      kissat_phase (solver, "walk", GET (walks),
		    "last irredundant clause reference %u too large",
		    last_irredundant);
      return kissat_rephase_best (solver);
    }

  if (IRREDUNDANT_CLAUSES > MAX_WALK_REF)
    {
      kissat_phase (solver, "walk", GET (walks),
		    "way too many irredundant clauses %" PRIu64,
		    IRREDUNDANT_CLAUSES);
      return kissat_rephase_best (solver);
    }

  STOP_SEARCH_AND_START_SIMPLIFIER (walking);
  char res = walking_phase (solver);
  STOP_SIMPLIFIER_AND_RESUME_SEARCH (walking);
  return res;
}

int
kissat_walk_initially (kissat * solver)
{
  if (solver->inconsistent)
    return 20;
  if (TERMINATED (25))
    return 0;
  if (!GET_OPTION (walkinitially))
    return 0;
#ifndef QUIET
  char type =
#endif
    kissat_walk (solver);
  REPORT (0, type ? type : 'W');
  return 0;
}
