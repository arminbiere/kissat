#include "allocate.h"
#include "analyze.h"
#include "backtrack.h"
#include "decide.h"
#include "failed.h"
#include "print.h"
#include "prophyper.h"
#include "proprobe.h"
#include "inline.h"
#include "sort.h"
#include "report.h"
#include "terminate.h"
#include "trail.h"

#include <inttypes.h>

static bool
occurs_in_binary_clause (kissat * solver, const value * values, unsigned lit)
{
  for (all_binary_blocking_watches (watch, WATCHES (lit)))
    {
      if (!watch.type.binary)
	continue;
      const unsigned other = watch.binary.lit;
      if (values[other])
	continue;
      return true;
    }
  return false;
}

static size_t
schedule_probes (kissat * solver, unsigned round, unsigneds * roots)
{
  assert (!solver->level);
  assert (EMPTY_STACK (*roots));
  LOG ("collecting roots of binary implication graph");
  assert (solver->watching);
  const value *values = solver->values;
  value *marks = solver->marks;
  flags *flags = solver->flags;
  for (all_variables (idx))
    {
      if (!flags[idx].active)
	continue;
      const unsigned lit = LIT (idx);
      const unsigned not_lit = NOT (lit);
      if (occurs_in_binary_clause (solver, values, lit))
	marks[lit] = true;
      if (occurs_in_binary_clause (solver, values, not_lit))
	marks[not_lit] = true;
    }
  for (all_clauses (c))
    {
      if (c->garbage)
	continue;
      unsigned size = 0, first = INVALID_LIT, second = INVALID_LIT;
      for (all_literals_in_clause (lit, c))
	{
	  const value value = values[lit];
	  if (value > 0)
	    {
	      kissat_mark_clause_as_garbage (solver, c);
	      size = UINT_MAX;
	      break;
	    }
	  if (value < 0)
	    continue;
	  if (!size++)
	    first = lit;
	  else if (size == 2)
	    second = lit;
	  else
	    break;
	}
      if (size > 2)
	continue;
      assert (size == 2);
      assert (first != INVALID_LIT);
      assert (second != INVALID_LIT);
      marks[first] = 1;
      marks[second] = 1;
    }
  unsigned scheduled[2] = { 0, 0 };
  for (all_variables (idx))
    {
      const unsigned lit = LIT (idx);
      const unsigned not_lit = NOT (lit);
      if (!flags[idx].active)
	continue;
      const bool pos = marks[lit];
      const bool neg = marks[not_lit];
      if (pos && neg)
	{
	  LOG ("skipping variable %u occurring positively and negatively",
	       idx);
	  continue;
	}
      if (!pos && !neg)
	{
	  LOG ("skipping variable %u not occurring at all", idx);
	  continue;
	}
      unsigned root = INVALID_LIT;
      if (pos && !neg)
	{
	  LOG ("variable %u occurs only positively", idx);
	  root = not_lit;
	}
      if (!pos && neg)
	{
	  LOG ("variable %u occurs only negatively", idx);
	  root = lit;
	}
      LOG ("collecting root %s", LOGLIT (root));
      PUSH_STACK (*roots, root);
      unsigned probe = flags[idx].probe;
      scheduled[probe]++;
    }
  memset (marks, 0, LITS * sizeof *marks);
  size_t res = SIZE_STACK (*roots);
  assert (res == scheduled[0] + scheduled[1]);
#ifdef QUIET
  (void) round;
#else
  kissat_phase (solver, "failed", GET (failed_literal_probings),
		"round %u: scheduled %zu probes %.0f%% out of all variables",
		round, res, kissat_percent (res, solver->active));
  if (scheduled[1])
    kissat_phase (solver, "failed", GET (failed_literal_probings),
		  "round %u: prioritized %u probes %.0f%% of all scheduled",
		  round, scheduled[1], kissat_percent (scheduled[1], res));
#endif
  return res;
}

static inline bool
less_stable_probe (kissat * solver,
		   const flags * flags, const heap * scores,
		   unsigned a, unsigned b)
{
#ifdef NDEBUG
  (void) solver;
#endif
  const unsigned i = IDX (a);
  const unsigned j = IDX (b);
  const bool p = flags[i].probe;
  const bool q = flags[j].probe;
  if (!p && q)
    return true;
  if (p && !q)
    return false;
  const double s = kissat_get_heap_score (scores, i);
  const double t = kissat_get_heap_score (scores, j);
  if (s < t)
    return true;
  if (s > t)
    return false;
  return i < j;
}

static inline unsigned
less_focused_probe (kissat * solver,
		    const flags * flags, const links * links,
		    unsigned a, unsigned b)
{
#ifdef NDEBUG
  (void) solver;
#endif
  const unsigned i = IDX (a);
  const unsigned j = IDX (b);
  const bool p = flags[i].probe;
  const bool q = flags[j].probe;
  if (!p && q)
    return true;
  if (p && !q)
    return false;
  const unsigned s = links[i].stamp;
  const unsigned t = links[j].stamp;
  return s < t;
}

#define LESS_STABLE_PROBE(A,B) \
  less_stable_probe (solver, flags, scores, (A), (B))

#define LESS_FOCUSED_PROBE(A,B) \
  less_focused_probe (solver, flags, links, (A), (B))

static void
sort_stable_probes (kissat * solver, unsigneds * probes)
{
  const flags *flags = solver->flags;
  const heap *scores = &solver->scores;
  SORT_STACK (unsigned, *probes, LESS_STABLE_PROBE);
}

static void
sort_focused_probes (kissat * solver, unsigneds * probes)
{
  const flags *flags = solver->flags;
  const links *links = solver->links;
  SORT_STACK (unsigned, *probes, LESS_FOCUSED_PROBE);
}

static void
sort_probes (kissat * solver, unsigneds * probes)
{
  if (solver->stable)
    sort_stable_probes (solver, probes);
  else
    sort_focused_probes (solver, probes);
}

static bool
probe_round (kissat * solver, unsigned round,
	     uint64_t limit, unsigneds * probes,
	     unsigned *failed_ptr, uint64_t * resolved_ptr)
{
#ifndef QUIET
  const unsigned scheduled = SIZE_STACK (*probes);
#endif
  unsigned *stamps = kissat_calloc (solver, LITS, sizeof *stamps);
  const value *values = solver->values;
  flags *flags = solver->flags;

#if !defined(NDEBUG) && !defined(NMETRICS)
  uint64_t resolved = solver->statistics.hyper_binary_resolved;
#endif
  uint64_t redundant = solver->statistics.clauses_redundant;

  unsigned probed = 0;
  unsigned failed = 0;

  while (!EMPTY_STACK (*probes))
    {
      const unsigned probe = POP_STACK (*probes);
      if (values[probe])
	continue;
      const unsigned probe_idx = IDX (probe);
      flags[probe_idx].probe = false;
      if (failed && stamps[probe] == failed)
	continue;
      if (solver->stable)
	LOG ("probing %s[%g]", LOGLIT (probe),
	     kissat_get_heap_score (&solver->scores, IDX (probe)));
      else
	LOG ("probing %s{%u}", LOGLIT (probe), LINK (IDX (probe)).stamp);
      probed++;
      const unsigned propagated = solver->propagated;
      kissat_internal_assume (solver, probe);
      assert (solver->level == 1);
      assert (values[probe] > 0);
      clause *conflict = kissat_hyper_propagate (solver, 0);
      if (conflict)
	{
	  LOG ("failed literal %s", LOGLIT (probe));
	  INC (failed);
	  failed++;
	  (void) kissat_analyze (solver, conflict);
	  assert (!solver->level);
	  assert (values[probe] <= 0);
	  if (!values[probe])
	    {
	      const unsigned unit = NOT (probe);
	      LOG ("learned unit %s from failed literal %s",
		   LOGLIT (unit), LOGLIT (probe));
	      kissat_assign_unit (solver, unit);
	      CHECK_AND_ADD_UNIT (unit);
	      ADD_UNIT_TO_PROOF (unit);
	    }
	  conflict = kissat_probing_propagate (solver, 0);
	  if (conflict)
	    {
	      (void) kissat_analyze (solver, conflict);
	      assert (solver->inconsistent);
	      break;
	    }
	  assert (!solver->level);
	  assert (solver->unflushed);
	  kissat_flush_trail (solver);
	}
      else
	{
	  assert (solver->level == 1);
	  assert (solver->propagated == SIZE_STACK (solver->trail));
	  for (unsigned i = propagated; i < solver->propagated; i++)
	    {
	      const unsigned lit = PEEK_STACK (solver->trail, i);
	      stamps[lit] = failed;
	    }
	  kissat_backtrack (solver, 0);
	}

      bool terminate;
      if (TERMINATED (7))
	{
	  LOG ("terminating");
	  terminate = true;
	}
      else if (solver->statistics.probing_ticks > limit)
	{
	  LOG ("ticks limit hit");
	  terminate = true;
	}
      else
	terminate = false;

      if (!terminate)
	continue;

#ifndef QUIET
      unsigned remain = SIZE_STACK (*probes);
#endif
      unsigned prioritized = 0;
      for (all_stack (unsigned, lit, *probes))
	if (flags[IDX (lit)].probe)
	    prioritized++;

      if (!prioritized)
	{
	  kissat_phase (solver, "failed",
			GET (failed_literal_probings),
			"round %u: prioritizing remaining %zu probes",
			round, remain);
	  while (!EMPTY_STACK (*probes))
	    {
	      const unsigned other = POP_STACK (*probes);
	      const unsigned other_idx = IDX (other);
	      assert (!flags[other_idx].probe);
	      flags[other_idx].probe = true;
	    }
	}
      else
	kissat_phase (solver, "failed",
		      GET (failed_literal_probings),
		      "round %u: keeping %zu probes %.0f%% prioritized",
		      round, prioritized,
		      kissat_percent (prioritized, remain));
      break;
    }
  kissat_dealloc (solver, stamps, LITS, sizeof *stamps);

  redundant = solver->statistics.clauses_redundant - redundant;
#if !defined(NDEBUG) && !defined(NMETRICS)
  assert (resolved <= solver->statistics.hyper_binary_resolved);
  resolved = solver->statistics.hyper_binary_resolved - resolved;
  assert (redundant == resolved);
#endif

#ifndef QUIET
  kissat_phase (solver, "failed",
		GET (failed_literal_probings),
		"round %u: %u failed (%.0f%%) "
		"of %u probed (%.0f%% scheduled)",
		round,
		failed, kissat_percent (failed, probed),
		probed, kissat_percent (probed, scheduled));

  kissat_phase (solver, "failed", GET (failed_literal_probings),
		"round %u: %" PRIu64 " hyper binary resolvents "
		"%.1f per probe", round, redundant,
		kissat_average (redundant, probed));
#else
  (void) round;
#endif
  *resolved_ptr += redundant;
  *failed_ptr = failed;

  if (solver->inconsistent)
    {
      kissat_phase (solver, "failed",
		    GET (failed_literal_probings),
		    "last round produced empty clause");
      return false;
    }

  if (!failed)
    kissat_phase (solver, "failed",
		  GET (failed_literal_probings),
		  "no failed literal produced in last round");
  if (!redundant)
    kissat_phase (solver, "failed",
		  GET (failed_literal_probings),
		  "no hyper binary resolvent produced in last round");
#ifndef QUIET
  const bool success = failed || redundant;
  REPORT (!success, 'f');
#endif
  return failed;
}

void
kissat_failed_literal_probing (kissat * solver)
{
  if (solver->inconsistent)
    return;

  assert (!solver->level);
  assert (solver->probing);
  assert (solver->watching);

  if (TERMINATED (8))
    return;
  if (!GET_OPTION (failed))
    return;

  RETURN_IF_DELAYED (failed);

  START (failed);
  INC (failed_literal_probings);

  if (solver->unflushed)
    kissat_flush_trail (solver);

  unsigneds roots;
  INIT_STACK (roots);

  SET_EFFICIENCY_BOUND (limit, failed, probing_ticks,
			search_ticks, kissat_nlogn (solver->active));

  const unsigned max_rounds = GET_OPTION (failedrounds);
  unsigned round = 1;

  uint64_t resolved = 0;
  unsigned failed = 0;

  for (;;)
    {
      size_t scheduled = schedule_probes (solver, round, &roots);
      if (!scheduled)
	{
	  kissat_phase (solver, "failed",
			GET (failed_literal_probings),
			"no roots found in binary implication graph");
	  break;
	}
      sort_probes (solver, &roots);
      if (!probe_round (solver, round, limit, &roots, &failed, &resolved))
	break;
      if (!EMPTY_STACK (roots))
	{
#ifndef QUIET
	  size_t remain = SIZE_STACK (roots);
	  kissat_phase (solver, "failed",
			GET (failed_literal_probings),
			"incomplete probing with %zu remaining %.0f%%",
			remain, kissat_percent (remain, scheduled));
#endif
	  break;
	}
      if (round == max_rounds)
	{
	  kissat_phase (solver, "failed",
			GET (failed_literal_probings),
			"completed probing until maximum round");
	  break;
	}
      round++;
    }
  RELEASE_STACK (roots);

  if (failed)
    kissat_phase (solver, "failed", GET (failed_literal_probings),
		  "%u failed literals in total in %u rounds", failed, round);
  else
    kissat_phase (solver, "failed", GET (failed_literal_probings),
		  "no failed literals in %u rounds", round);

  if (resolved)
    kissat_phase (solver, "failed", GET (failed_literal_probings),
		  "%" PRIu64 " hyper binary resolvents in total in %u rounds",
		  resolved, round);
  else
    kissat_phase (solver, "failed", GET (failed_literal_probings),
		  "no hyper binary resolvents in total in %u rounds", round);

  bool success = failed + resolved;
  UPDATE_DELAY (success, failed);

  STOP (failed);
}
