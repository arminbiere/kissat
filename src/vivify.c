#include "vivify.h"
#include "allocate.h"
#include "backtrack.h"
#include "colors.h"
#include "decide.h"
#include "inline.h"
#include "print.h"
#include "promote.h"
#include "proprobe.h"
#include "report.h"
#include "sort.h"
#include "terminate.h"
#include "trail.h"

#include <inttypes.h>

static inline bool more_occurrences (unsigned *counts, unsigned a,
                                     unsigned b) {
  const unsigned s = counts[a], t = counts[b];
  return ((t - s) | ((b - a) & ~(s - t))) >> 31;
}

#define MORE_OCCURRENCES(A, B) more_occurrences (counts, (A), (B))

static void vivify_sort_lits_by_counts (kissat *solver, size_t size,
                                        unsigned *lits, unsigned *counts) {
  SORT (unsigned, size, lits, MORE_OCCURRENCES);
}

static void vivify_sort_stack_by_counts (kissat *solver, unsigneds *stack,
                                         unsigned *counts) {
  const size_t size = SIZE_STACK (*stack);
  unsigned *lits = BEGIN_STACK (*stack);
  vivify_sort_lits_by_counts (solver, size, lits, counts);
}

static void vivify_sort_clause_by_counts (kissat *solver, clause *c,
                                          unsigned *counts) {
  vivify_sort_lits_by_counts (solver, c->size, c->lits, counts);
}

static inline void count_literal (unsigned lit, unsigned *counts) {
  counts[lit] += counts[lit] < (unsigned) INT_MAX;
}

static void count_clause (clause *c, unsigned *counts) {
  for (all_literals_in_clause (lit, c))
    count_literal (lit, counts);
}

static bool simplify_vivification_candidate (kissat *solver,
                                             clause *const c) {
  assert (!solver->level);
  bool satisfied = false;
  assert (EMPTY_STACK (solver->clause));
  const value *const values = solver->values;
  for (all_literals_in_clause (lit, c)) {
    const value value = values[lit];
    if (value > 0) {
      satisfied = true;
      LOGCLS (c, "vivification %s satisfied candidate", LOGLIT (lit));
      kissat_mark_clause_as_garbage (solver, c);
      break;
    }
    if (!value)
      PUSH_STACK (solver->clause, lit);
  }
  unsigned non_false = SIZE_STACK (solver->clause);
  if (satisfied) {
    CLEAR_STACK (solver->clause);
    return true;
  }
  if (non_false == c->size) {
    CLEAR_STACK (solver->clause);
    return false;
  }
  assert (1 < non_false);
  assert (non_false <= c->size);
  if (non_false == 2) {
    const unsigned first = PEEK_STACK (solver->clause, 0);
    const unsigned second = PEEK_STACK (solver->clause, 1);
    LOGBINARY (first, second, "vivification shrunken candidate");
    kissat_new_binary_clause (solver, first, second);
    kissat_mark_clause_as_garbage (solver, c);
  } else {
    CHECK_AND_ADD_STACK (solver->clause);
    ADD_STACK_TO_PROOF (solver->clause);

    REMOVE_CHECKER_CLAUSE (c);
    DELETE_CLAUSE_FROM_PROOF (c);

    const unsigned old_size = c->size;
    unsigned new_size = 0, *lits = c->lits;
    for (unsigned i = 0; i < old_size; i++) {
      const unsigned lit = lits[i];
      const value value = kissat_fixed (solver, lit);
      assert (value <= 0);
      if (value < 0)
        continue;
      lits[new_size++] = lit;
    }
    assert (2 < new_size);
    assert (new_size == non_false);
    assert (new_size < old_size);
    c->size = new_size;
    c->searched = 2;
    if (c->redundant && c->glue >= new_size)
      kissat_promote_clause (solver, c, new_size - 1);
    if (!c->shrunken) {
      c->shrunken = true;
      lits[old_size - 1] = INVALID_LIT;
    }
    LOGCLS (c, "vivification shrunken candidate");
  }
  CLEAR_STACK (solver->clause);
  return false;
}

static void schedule_vivification_candidates (kissat *solver,
#ifndef QUIET
                                              const char *mode,
#endif
                                              references *const schedule,
                                              unsigned *const counts,
                                              int tier) {
  unsigned lower_glue_limit, upper_glue_limit;
  lower_glue_limit = tier == 2 ? GET_OPTION (tier1) + 1 : 0;
  upper_glue_limit = tier == 2 ? GET_OPTION (tier2) : GET_OPTION (tier1);
  ward *const arena = BEGIN_STACK (solver->arena);
  size_t prioritized = 0;
  for (unsigned prioritize = 0; prioritize < 2; prioritize++) {
    for (all_clauses (c)) {
      if (c->garbage)
        continue;
      if (prioritize)
        count_clause (c, counts);
      if (tier) {
        if (!c->redundant)
          continue;
        if (c->glue < lower_glue_limit)
          continue;
        if (c->glue > upper_glue_limit)
          continue;
      } else if (c->redundant)
        continue;
      if (c->vivify != prioritize)
        continue;
      if (simplify_vivification_candidate (solver, c))
        continue;
      if (prioritize)
        prioritized++;
      const reference ref = (ward *) c - arena;
      PUSH_STACK (*schedule, ref);
    }
  }
#ifndef QUIET
  size_t scheduled = SIZE_STACK (*schedule);
#endif
  if (prioritized) {
    kissat_phase (solver, mode, GET (vivifications),
                  "prioritized %zu clauses %.0f%%", prioritized,
                  kissat_percent (prioritized, scheduled));
  } else {
    kissat_phase (solver, mode, GET (vivifications),
                  "prioritizing all %zu scheduled clauses", scheduled);
    for (all_stack (reference, ref, *schedule)) {
      clause *c = (clause *) (arena + ref);
      assert (kissat_clause_in_arena (solver, c));
      c->vivify = true;
    }
  }
}

static inline bool worse_candidate (kissat *solver, unsigned *counts,
                                    reference r, reference s) {
  const clause *const c = kissat_dereference_clause (solver, r);
  const clause *const d = kissat_dereference_clause (solver, s);

  if (!c->vivify && d->vivify)
    return true;

  if (c->vivify && !d->vivify)
    return false;

  unsigned const *p = BEGIN_LITS (c);
  unsigned const *q = BEGIN_LITS (d);
  const unsigned *const e = END_LITS (c);
  const unsigned *const f = END_LITS (d);

  while (p != e && q != f) {
    const unsigned a = *p++;
    const unsigned b = *q++;
    const unsigned u = counts[a];
    const unsigned v = counts[b];
    if (u < v)
      return true;
    if (u > v)
      return false;
    if (a < b)
      return true;
    if (a > b)
      return false;
  }

  if (p != e && q == f)
    return false;

  if (p == e && q != f)
    return true;

  return r < s;
}

#define WORSE_CANDIDATE(A, B) worse_candidate (solver, counts, (A), (B))

static void sort_vivification_candidates (kissat *solver,
                                          references *schedule,
                                          unsigned *counts) {
  for (all_stack (reference, ref, *schedule)) {
    clause *c = kissat_dereference_clause (solver, ref);
    vivify_sort_clause_by_counts (solver, c, counts);
  }
  SORT_STACK (reference, *schedule, WORSE_CANDIDATE);
}

static bool vivify_deduce (kissat *solver, clause *candidate,
                           clause *conflict, unsigned implied) {
  bool redundant = false;

  assert (solver->level);
  assert (EMPTY_STACK (solver->clause));
  assert (EMPTY_STACK (solver->analyzed));

  if (implied != INVALID_LIT) {
    unsigned not_implied = NOT (implied);
    LOG ("vivify analyzing %s", LOGLIT (not_implied));
    assigned *const a = ASSIGNED (not_implied);
    assert (a->level);
    assert (!a->analyzed);
    a->analyzed = true;
    PUSH_STACK (solver->analyzed, not_implied);
    PUSH_STACK (solver->clause, implied);
  } else {
    clause *reason = conflict ? conflict : candidate;
    assert (reason), assert (!reason->garbage);
    if (reason->redundant)
      redundant = true;
    for (all_literals_in_clause (other, reason)) {
      assert (solver->values[other] < 0);
      const value value = kissat_fixed (solver, other);
      if (value < 0)
        continue;
      LOG ("vivify analyzing %s", LOGLIT (other));
      assert (!value);
      assigned *const a = ASSIGNED (other);
      assert (a->level);
      assert (!a->analyzed);
      a->analyzed = true;
      PUSH_STACK (solver->analyzed, other);
    }
  }

  size_t analyzed = 0;
  while (analyzed < SIZE_STACK (solver->analyzed)) {
    const unsigned not_lit = PEEK_STACK (solver->analyzed, analyzed);
    const unsigned lit = NOT (not_lit);
    analyzed++;
    assigned *a = ASSIGNED (lit);
    assert (a->level);
    assert (a->analyzed);
    if (a->reason == DECISION_REASON) {
      LOG ("vivify analyzing decision %s", LOGLIT (not_lit));
      PUSH_STACK (solver->clause, not_lit);
    } else if (a->binary) {
      const unsigned other = a->reason;
      assert (VALUE (other) < 0);
      assigned *b = ASSIGNED (other);
      assert (b->level);
      if (b->analyzed)
        continue;
      LOGBINARY (lit, other, "vivify analyzing %s reason", LOGLIT (lit));
      b->analyzed = true;
      PUSH_STACK (solver->analyzed, other);
    } else {
      const reference ref = a->reason;
      LOGREF (ref, "vivify analyzing %s reason", LOGLIT (lit));
      clause *reason = kissat_dereference_clause (solver, ref);
      assert (reason != candidate);
      if (reason->redundant)
        redundant = true;
      for (all_literals_in_clause (other, reason)) {
        if (other == lit)
          continue;
        if (other == not_lit)
          continue;
        assert (VALUE (other) < 0);
        assigned *b = ASSIGNED (other);
        if (!b->level)
          continue;
        if (b->analyzed)
          continue;
        b->analyzed = true;
        PUSH_STACK (solver->analyzed, other);
      }
    }
  }

  LOGTMP ("vivify %sredundantly deduced", redundant ? "" : "ir");
  return redundant;
}

static void reset_vivify_analyzed (kissat *solver) {
  struct assigned *assigned = solver->assigned;
  for (all_stack (unsigned, lit, solver->analyzed)) {
    const unsigned idx = IDX (lit);
    struct assigned *a = assigned + idx;
    a->analyzed = false;
  }
  CLEAR_STACK (solver->analyzed);
  CLEAR_STACK (solver->clause);
}

static bool vivify_shrink (kissat *solver, clause *c, bool *subsumed_ptr) {
  const value *const values = solver->values;
  const assigned *const assigned = solver->assigned;
  bool subsumed = false;
  for (all_literals_in_clause (lit, c)) {
    value value = values[lit];
    if (!value) {
      LOG ("vivification removes at least unassigned %s", LOGLIT (lit));
      return true;
    }
    if (value > 0) {
      LOG ("vivification implied satisfied %s", LOGLIT (lit));
      subsumed = true;
      continue;
    }
    const unsigned idx = IDX (lit);
    const struct assigned *const a = assigned + idx;
    if (a->level && !a->analyzed) {
      LOG ("vivification removes at least non-analyzed %s", LOGLIT (lit));
      return true;
    }
  }
  *subsumed_ptr = subsumed;
  return false;
}

static void vivify_learn (kissat *solver, clause *c, unsigned implied) {
  size_t size = SIZE_STACK (solver->clause);

  if (size == 1) {
    LOG ("size 1 learned unit clause forces jump level 0");
    if (solver->level)
      kissat_backtrack_without_updating_phases (solver, 0);

    const unsigned unit = PEEK_STACK (solver->clause, 0);
    kissat_learned_unit (solver, unit);
    solver->iterating = true;
    kissat_mark_clause_as_garbage (solver, c);
    assert (!solver->level);
    (void) kissat_probing_propagate (solver, 0, true);
    INC (vivify_units);
  } else {
    const assigned *const assigned = solver->assigned;
    const value *const values = solver->values;

    unsigned highest_level = 0;
    for (all_stack (unsigned, lit, solver->clause)) {
      const value value = values[lit];
      if (!value) {
        LOG ("unassigned literal %s in learned clause", LOGLIT (lit));
        highest_level = INVALID_LEVEL;
        break;
      }
      const unsigned idx = IDX (lit);
      const struct assigned *a = assigned + idx;
      const unsigned level = a->level;
      assert (level > 0);
      if (level > highest_level)
        highest_level = level;
    }

    if (highest_level != INVALID_LEVEL)
      LOG ("highest level %u in learned clause", highest_level);

    unsigned literals_on_highest_level = 0;
    for (all_stack (unsigned, lit, solver->clause)) {
      const value value = values[lit];
      if (!value)
        literals_on_highest_level++;
      else {
        const unsigned idx = IDX (lit);
        const struct assigned *a = assigned + idx;
        const unsigned level = a->level;
        assert (level > 0);
        if (level == highest_level)
          literals_on_highest_level++;
      }
    }
#ifdef LOGGING
    if (highest_level == INVALID_LEVEL)
      LOG ("found %u unassigned literals", literals_on_highest_level);
    else
      LOG ("found %u literals on highest level", literals_on_highest_level);
#endif
    if (highest_level == INVALID_LEVEL && literals_on_highest_level > 1)
      LOG ("no need to backtrack with more than one unassigned literal");
    else {
      unsigned jump_level = 0;
      for (all_stack (unsigned, lit, solver->clause)) {
        const value value = values[lit];
        if (!value)
          continue;
        const unsigned idx = IDX (lit);
        const struct assigned *a = assigned + idx;
        const unsigned level = a->level;
        if (level == highest_level)
          continue;
        if (level > jump_level)
          jump_level = level;
      }
      LOG ("determined jump level %u", jump_level);

      if (jump_level < solver->level)
        kissat_backtrack_without_updating_phases (solver, jump_level);
    }

    if (size == 2) {
      if (c->redundant)
        (void) kissat_new_redundant_clause (solver, 1);
      else
        (void) kissat_new_irredundant_clause (solver);
      kissat_mark_clause_as_garbage (solver, c);
    } else {
      CHECK_AND_ADD_STACK (solver->clause);
      ADD_STACK_TO_PROOF (solver->clause);

      REMOVE_CHECKER_CLAUSE (c);
      DELETE_CLAUSE_FROM_PROOF (c);

      assert (size > 2);
      bool irredundant = !c->redundant;
      const unsigned old_size = c->size;
      unsigned new_size = 0, *lits = c->lits;
      unsigned watched[2] = {lits[0], lits[1]};
      for (unsigned i = 0; i < old_size; i++) {
        const unsigned lit = lits[i];
        bool keep = true;
        if (lit != implied) {
          const unsigned idx = IDX (lit);
          const struct assigned *a = assigned + idx;
          if (!a->analyzed)
            keep = false;
          else if (a->reason != DECISION_REASON)
            keep = false;
        }
        if (keep) {
          lits[new_size++] = lit;
          if (irredundant)
            kissat_mark_added_literal (solver, lit);
        } else if (irredundant)
          kissat_mark_removed_literal (solver, lit);
      }
      assert (new_size < old_size);
      assert (new_size == size);
      if (!c->shrunken) {
        c->shrunken = true;
        lits[old_size - 1] = INVALID_LIT;
      }
      c->size = new_size;
      if (!irredundant && c->glue >= new_size)
        kissat_promote_clause (solver, c, new_size - 1);
      c->searched = 2;
      LOGCLS (c, "vivified shrunken");

      const reference ref = kissat_reference_clause (solver, c);

      // Beware of 'stale blocking literals' ... so rewatch if shrunken.

      kissat_unwatch_blocking (solver, watched[0], ref);
      kissat_unwatch_blocking (solver, watched[1], ref);
      kissat_watch_blocking (solver, lits[0], lits[1], ref);
      kissat_watch_blocking (solver, lits[1], lits[0], ref);
    }
  }
}

static bool vivify_clause (kissat *solver, clause *candidate,
                           unsigneds *sorted, unsigned *counts) {
  assert (!candidate->garbage);
  assert (solver->probing);
  assert (solver->watching);
  assert (!solver->inconsistent);

  LOGCLS (candidate, "vivifying candidate");
  LOGCOUNTEDCLS (candidate, counts, "vivifying unsorted counted candidate");

  CLEAR_STACK (*sorted);

  for (all_literals_in_clause (lit, candidate)) {
    const value value = kissat_fixed (solver, lit);
    if (value < 0)
      continue;
    if (value > 0) {
      LOGCLS (candidate, "%s satisfied", LOGLIT (lit));
      kissat_mark_clause_as_garbage (solver, candidate);
      break;
    }
    PUSH_STACK (*sorted, lit);
  }

  if (candidate->garbage)
    return false;

  const unsigned non_false = SIZE_STACK (*sorted);

  assert (1 < non_false);
  assert (non_false <= candidate->size);

#ifdef LOGGING
  if (!non_false)
    LOG ("no root level falsified literal");
  else if (non_false == candidate->size)
    LOG ("all literals root level unassigned");
  else
    LOG ("found %u root level non-falsified literals", non_false);
#endif

  if (non_false == 2) {
    LOGCLS (candidate, "skipping actually binary");
    return false;
  }

  INC (vivify_checks);

  unsigned unit = INVALID_LIT;
  for (all_literals_in_clause (lit, candidate)) {
    const value value = VALUE (lit);
    if (value < 0)
      continue;
    if (!value) {
      unit = INVALID_LIT;
      break;
    }
    assert (value > 0);
    if (unit != INVALID_LIT) {
      unit = INVALID_LIT;
      break;
    }
    unit = lit;
  }
  if (unit != INVALID_LIT) {
    assigned *a = ASSIGNED (unit);
    assert (a->level);
    if (a->binary)
      unit = INVALID_LIT;
    else {
      reference ref = kissat_reference_clause (solver, candidate);
      if (a->reason != ref)
        unit = INVALID_LIT;
    }
  }
  if (unit == INVALID_LIT)
    LOG ("non-reason candidate clause");
  else {
    LOG ("candidate is the reason of %s", LOGLIT (unit));
    const unsigned level = LEVEL (unit);
    assert (level > 0);
    LOG ("forced to backtrack to level %u", level - 1);
    kissat_backtrack_without_updating_phases (solver, level - 1);
  }

  assert (EMPTY_STACK (solver->analyzed));
  assert (EMPTY_STACK (solver->clause));

  vivify_sort_stack_by_counts (solver, sorted, counts);

  LOGCOUNTEDLITS (SIZE_STACK (*sorted), sorted->begin, counts,
                  "vivifying counted sorted candidate");

  unsigned implied = INVALID_LIT;
  clause *conflict = 0;
  unsigned level = 0;

  for (all_stack (unsigned, lit, *sorted)) {
    if (level++ < solver->level) {
      frame *frame = &FRAME (level);
      const unsigned not_lit = NOT (lit);
      if (frame->decision == not_lit) {
        LOG ("reusing assumption %s", LOGLIT (not_lit));
        INC (vivify_reused);
        INC (vivify_probes);
        assert (VALUE (lit) < 0);
        continue;
      }

      LOG ("forced to backtrack to decision level %u", level - 1);
      kissat_backtrack_without_updating_phases (solver, level - 1);
    }

    const value value = VALUE (lit);
    assert (!value || LEVEL (lit) <= level);

    if (value < 0) {
      assert (LEVEL (lit));
      LOG ("literal %s already falsified", LOGLIT (lit));
      continue;
    }

    if (value > 0) {
      assert (LEVEL (lit));
      LOG ("literal %s already satisfied", LOGLIT (lit));
      implied = lit;
      break;
    }

    assert (!value);

    LOG ("literal %s unassigned", LOGLIT (lit));
    const unsigned not_lit = NOT (lit);
    INC (vivify_probes);

    kissat_internal_assume (solver, not_lit);
    assert (solver->level >= 1);

    conflict = kissat_probing_propagate (solver, candidate, true);
    if (conflict)
      break;
  }

  bool redundant = vivify_deduce (solver, candidate, conflict, implied);
  bool subsumed;
  bool res = vivify_shrink (solver, candidate, &subsumed);
  if (res)
    vivify_learn (solver, candidate, implied);
  else if (subsumed && !redundant && !candidate->redundant) {
    LOGCLS (candidate, "vivification subsumed");
    kissat_mark_removed_literals (solver, candidate->size, candidate->lits);
    assert (solver->statistics.clauses_irredundant);
    solver->statistics.clauses_irredundant--;
    assert (solver->statistics.clauses_redundant < UINT64_MAX);
    solver->statistics.clauses_redundant++;
    candidate->glue = 0;
    candidate->redundant = true;
    LOGCLS (candidate, "vivification demoted");
    res = true;
  } else
    LOGCLS (candidate, "vivification failed on");

  reset_vivify_analyzed (solver);

  if (res)
    INC (vivified);

  return res;
}

static void vivify_round (kissat *solver, int tier, uint64_t limit) {
  if (tier && !REDUNDANT_CLAUSES)
    return;

  assert (0 <= tier && tier <= 2);
  assert (solver->watching);
  assert (solver->probing);

#ifndef QUIET
  const char *mode;
  char tag;
  switch (tier) {
  case 1:
    mode = "vivify-tier1";
    tag = 'u';
    break;
  case 2:
    mode = "vivify-tier2";
    tag = 'v';
    break;
  default:
    assert (tier == 0);
    mode = "vivify-irredundant";
    tag = 'w';
    break;
  }
#endif

  references schedule;
  INIT_STACK (schedule);

  kissat_flush_large_watches (solver);

  unsigned *counts = kissat_calloc (solver, LITS, sizeof (unsigned));

  schedule_vivification_candidates (solver,
#ifndef QUIET
                                    mode,
#endif
                                    &schedule, counts, tier);

  sort_vivification_candidates (solver, &schedule, counts);
  kissat_watch_large_clauses (solver);
#ifndef QUIET
  uint64_t start = solver->statistics.probing_ticks;
  uint64_t delta = limit - start;
  kissat_very_verbose (solver,
                       "%s effort limit %" PRIu64 " = %" PRIu64
                       " + %" PRIu64 " 'probing_ticks'",
                       mode, limit, start, delta);
  const size_t total = REDUNDANT_CLAUSES;
  const size_t scheduled = SIZE_STACK (schedule);
  kissat_phase (solver, mode, GET (vivifications),
                "scheduled %zu clauses %.0f%% of %zu", scheduled,
                kissat_percent (scheduled, total), total);
  size_t vivified = 0, tried = 0;
#endif
  unsigneds sorted;
  INIT_STACK (sorted);
  while (!EMPTY_STACK (schedule)) {
    const uint64_t probing_ticks = solver->statistics.probing_ticks;
    if (probing_ticks > limit) {
      kissat_extremely_verbose (solver,
                                "%s ticks limit %" PRIu64
                                " hit after %" PRIu64 " 'probing_ticks'",
                                mode, limit, probing_ticks);
      break;
    }
    if (TERMINATED (vivify_terminated_1))
      break;
    const reference ref = POP_STACK (schedule);
    clause *candidate = kissat_dereference_clause (solver, ref);
    if (candidate->garbage)
      continue;
#ifndef QUIET
    tried++;
#endif
    if (vivify_clause (solver, candidate, &sorted, counts)) {
#ifndef QUIET
      vivified++;
#endif
      if (!candidate->garbage && (tier || !candidate->redundant))
        PUSH_STACK (schedule, ref);
    }
    candidate->vivify = false;
    if (solver->inconsistent)
      break;
  }
  if (solver->level)
    kissat_backtrack_without_updating_phases (solver, 0);
  kissat_dealloc (solver, counts, LITS, sizeof *counts);
  RELEASE_STACK (sorted);
#ifndef QUIET
  kissat_phase (solver, mode, GET (vivifications),
                "vivified %zu clauses %.0f%% out of %zu tried", vivified,
                kissat_percent (vivified, tried), tried);
  if (!solver->inconsistent) {
    size_t remain = SIZE_STACK (schedule);
    if (remain) {
      kissat_phase (solver, mode, GET (vivifications),
                    "%zu clauses remain %.0f%% out of %zu scheduled",
                    remain, kissat_percent (remain, scheduled), scheduled);

      ward *const arena = BEGIN_STACK (solver->arena);
      size_t prioritized = 0;
      while (!EMPTY_STACK (schedule)) {
        const unsigned ref = POP_STACK (schedule);
        clause *c = (clause *) (arena + ref);
        if (c->vivify)
          prioritized++;
      }
      if (!prioritized)
        kissat_phase (solver, mode, GET (vivifications),
                      "no prioritized clauses left");
      else
        kissat_phase (solver, mode, GET (vivifications),
                      "keeping %zu clauses prioritized %.0f%%", prioritized,
                      kissat_percent (prioritized, remain));
    } else
      kissat_phase (solver, mode, GET (vivifications),
                    "all scheduled clauses tried");
  }
#endif
  RELEASE_STACK (schedule);
  REPORT (!vivified, tag);
}

void kissat_vivify (kissat *solver) {
  if (solver->inconsistent)
    return;
  assert (!solver->level);
  assert (solver->probing);
  assert (solver->watching);
  if (!GET_OPTION (vivify))
    return;
  double tier1 = GET_OPTION (vivifytier1);
  double tier2 = GET_OPTION (vivifytier2);
  double irr = GET_OPTION (vivifyirr);
  if (!REDUNDANT_CLAUSES)
    tier1 = tier2 = 0;
  double sum = tier1 + tier2 + irr;
  if (!sum)
    sum = irr = 1;
  START (vivify);
  INC (vivifications);
#if !defined(NDEBUG) || defined(METRICS)
  assert (!solver->vivifying);
  solver->vivifying = true;
#endif
  SET_EFFORT_LIMIT (limit, vivify, probing_ticks);
  const uint64_t total = limit - solver->statistics.probing_ticks;
  limit = solver->statistics.probing_ticks + (total * tier1) / sum;
  vivify_round (solver, 1, limit);
  if (!solver->inconsistent && !TERMINATED (vivify_terminated_2)) {
    limit += (total * tier2) / sum;
    vivify_round (solver, 2, limit);
    if (!solver->inconsistent && !TERMINATED (vivify_terminated_3)) {
      limit += (total * irr) / sum;
      vivify_round (solver, 0, limit);
    }
  }
#if !defined(NDEBUG) || defined(METRICS)
  assert (solver->vivifying);
  solver->vivifying = false;
#endif
  STOP (vivify);
}
