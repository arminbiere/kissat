#ifndef _statistics_h_INCLUDED
#define _statistics_h_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#define METRICS_COUNTERS_AND_STATISTICS \
\
  METRIC (allocated_collected, 2, PCNT_RESIDENT_SET, "%", "resident set") \
  METRIC (allocated_current, 2, PCNT_RESIDENT_SET, "%", "resident set") \
  METRIC (allocated_max, 2, PCNT_RESIDENT_SET, "%", "resident set") \
  STATISTIC (ands_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
  METRIC (ands_extracted, 1, PCNT_EXTRACTED, "%", "extracted") \
  METRIC (arena_enlarged, 1, PCNT_ARENA_RESIZED, "%", "resize") \
  METRIC (arena_garbage, 1, PCNT_RESIDENT_SET, "%", "resident set") \
  METRIC (arena_resized, 1, CONF_INT, "", "interval") \
  METRIC (arena_shrunken, 1, PCNT_ARENA_RESIZED, "%", "resize") \
  COUNTER (backbone_computations, 2, CONF_INT, 0, "interval") \
  METRIC (backbone_implied, 1, PER_BACKBONE_UNIT, 0, "per unit") \
  METRIC (backbone_probes, 2, PER_VARIABLE, "", "per variable") \
  METRIC (backbone_propagations, 2, PCNT_PROPS, "%", "propagations") \
  METRIC (backbone_rounds, 2, PER_BACKBONE, 0, "per backbone") \
  COUNTER (backbone_ticks, 2, PCNT_TICKS, "%", "ticks") \
  STATISTIC (backbone_units, 1, PCNT_VARIABLES, "%", "variables") \
  METRIC (best_saved, 1, CONF_INT, "", "interval") \
  COUNTER (chronological, 1, PCNT_CONFLICTS, "%", "conflicts") \
  METRIC (clauses_added, 2, PCNT_CLS_ADDED, "%", "added") \
  COUNTER (clauses_binary, 2, NO_SECONDARY, 0, 0) \
  METRIC (clauses_deleted, 2, PCNT_CLS_ADDED, "%", "added") \
  METRIC (clauses_improved, 2, PCNT_CLS_ADDED, "%", "added") \
  COUNTER (clauses_irredundant, 2, NO_SECONDARY, 0, 0) \
  METRIC (clauses_kept2, 2, PCNT_CLS_ADDED, "%", "added") \
  METRIC (clauses_kept3, 2, PCNT_CLS_ADDED, "%", "added") \
  METRIC (clauses_learned, 1, PCNT_CONFLICTS, "%", "conflicts") \
  METRIC (clauses_original, 2, PCNT_CLS_ADDED, "%", "added") \
  METRIC (clauses_promoted1, 2, PCNT_CLS_ADDED, "%", "added") \
  METRIC (clauses_promoted2, 2, PCNT_CLS_ADDED, "%", "added") \
  METRIC (clauses_reduced, 2, PCNT_CLS_ADDED, "%", "added") \
  COUNTER (clauses_redundant, 2, NO_SECONDARY, 0, 0) \
  METRIC (compacted, 1, PCNT_REDUCTIONS, "%", "reductions") \
  COUNTER (conflicts, 0, PER_SECOND, 0, "per second") \
  COUNTER (decisions, 0, PER_CONFLICT, 0, "per conflict") \
  METRIC (definitions_checked, 1, PCNT_ELIM_ATTEMPTS, "%", "attempts") \
  STATISTIC (definitions_eliminated, 1, PCNT_ELIMINATED, "%", \
             "eliminated") \
  METRIC (definitions_extracted, 1, PCNT_EXTRACTED, "%", "extracted") \
  STATISTIC (definition_units, 1, PCNT_VARIABLES, "%", "variables") \
  METRIC (defragmentations, 1, CONF_INT, "", "interval") \
  METRIC (dense_garbage_collections, 2, PCNT_COLLECTIONS, "%", \
          "collections") \
  METRIC (dense_propagations, 1, PCNT_PROPS, "%", "propagations") \
  METRIC (dense_ticks, 1, PCNT_TICKS, "%", "ticks") \
  METRIC (duplicated, 1, PCNT_CLS_ADDED, "%", "added") \
  COUNTER (eliminated, 1, PCNT_VARIABLES, "%", "variables") \
  COUNTER (eliminations, 2, CONF_INT, "", "interval") \
  STATISTIC (eliminate_attempted, 1, PER_VARIABLE, 0, "per variable") \
  COUNTER (eliminate_resolutions, 2, PER_SECOND, 0, "per second") \
  STATISTIC (eliminate_units, 1, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (equivalences_eliminated, 1, PCNT_ELIMINATED, "%", \
             "eliminated") \
  METRIC (equivalences_extracted, 1, PCNT_EXTRACTED, "%", "extracted") \
  METRIC (extensions, 1, PCNT_SEARCHES, "%", "searches") \
  STATISTIC (flipped, 1, PER_WALKS, 0, "per walk") \
  METRIC (flushed, 2, PER_FIXED, 0, "per fixed") \
  METRIC (focused_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  METRIC (focused_modes, 1, CONF_INT, "", "interval") \
  METRIC (focused_propagations, 1, PCNT_PROPS, "%", "propagations") \
  METRIC (focused_restarts, 1, PCNT_RESTARTS, "%", "restarts") \
  METRIC (focused_ticks, 1, PCNT_TICKS, "%", "ticks") \
  COUNTER (forward_checks, 2, NO_SECONDARY, 0, 0) \
  COUNTER (forward_steps, 2, PER_FORWARD_CHECK, "", "per check") \
  METRIC (forward_subsumptions, 1, CONF_INT, "", "interval") \
  METRIC (garbage_collections, 2, CONF_INT, "", "interval") \
  METRIC (gates_checked, 1, PCNT_ELIM_ATTEMPTS, "%", "attempts") \
  STATISTIC (gates_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
  METRIC (gates_extracted, 1, PCNT_ELIM_ATTEMPTS, "%", "attempts") \
  STATISTIC (if_then_else_eliminated, 1, PCNT_ELIMINATED, "%", \
             "eliminated") \
  METRIC (if_then_else_extracted, 1, PCNT_EXTRACTED, "%", "extracted") \
  METRIC (initial_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  COUNTER (jumped_reasons, 1, PCNT_PROPS, "%", "propagations") \
  STATISTIC (kitten_conflicts, 1, PER_KITTEN_SOLVED, 0, "per solved") \
  STATISTIC (kitten_decisions, 1, PER_KITTEN_SOLVED, 0, "per solved") \
  STATISTIC (kitten_flip, 1, NO_SECONDARY, 0, 0) \
  STATISTIC (kitten_flipped, 1, PCNT_KITTEN_FLIP, "%", "per flip") \
  COUNTER (kitten_propagations, 2, PER_KITTEN_SOLVED, 0, "per solved") \
  STATISTIC (kitten_sat, 1, PCNT_KITTEN_SOLVED, "%", "solved") \
  COUNTER (kitten_solved, 2, NO_SECONDARY, 0, 0) \
  COUNTER (kitten_ticks, 2, PER_KITTEN_PROP, 0, "per prop") \
  STATISTIC (kitten_unknown, 1, PCNT_KITTEN_SOLVED, "%", "solved") \
  STATISTIC (kitten_unsat, 1, PCNT_KITTEN_SOLVED, "%", "solved") \
  STATISTIC (learned_units, 1, PCNT_VARIABLES, "%", "variables") \
  METRIC (literals_bumped, 1, PER_CLS_LEARNED, 0, "per clause") \
  METRIC (literals_deduced, 1, PER_CLS_LEARNED, 0, "per clause") \
  METRIC (literals_learned, 1, PER_CLS_LEARNED, 0, "per clause") \
  METRIC (literals_minimized, 1, PCNT_LITS_DEDUCED, "%", "deduced") \
  METRIC (literals_minimize_shrunken, 1, PCNT_LITS_SHRUNKEN, "%", \
          "shrunken") \
  METRIC (literals_shrunken, 1, PCNT_LITS_DEDUCED, "%", "deduced") \
  METRIC (moved, 1, PCNT_REDUCTIONS, "%", "reductions") \
  METRIC (on_the_fly_strengthened, 1, PCNT_CONFLICTS, "%", "of conflicts") \
  METRIC (on_the_fly_subsumed, 1, PCNT_CONFLICTS, "%", "of conflicts") \
  METRIC (probing_propagations, 1, PCNT_PROPS, "%", "propagations") \
  COUNTER (probings, 2, CONF_INT, "", "interval") \
  COUNTER (probing_ticks, 2, PCNT_TICKS, "%", "ticks") \
  COUNTER (propagations, 0, PER_SECOND, "", "per second") \
  COUNTER (queue_decisions, 0, PCNT_DECISIONS, "%", "decision") \
  COUNTER (random_decisions, 0, PCNT_DECISIONS, "%", "decision") \
  COUNTER (random_sequences, 0, CONF_INT, "", "interval") \
  COUNTER (reductions, 1, CONF_INT, "", "interval") \
  COUNTER (rephased, 1, CONF_INT, "", "interval") \
  METRIC (rephased_best, 1, PCNT_REPHASED, "%", "rephased") \
  METRIC (rephased_inverted, 1, PCNT_REPHASED, "%", "rephased") \
  METRIC (rephased_original, 1, PCNT_REPHASED, "%", "rephased") \
  METRIC (rephased_walking, 1, PCNT_REPHASED, "%", "rephased") \
  METRIC (rescaled, 2, CONF_INT, "", "interval") \
  COUNTER (restarts, 1, CONF_INT, 0, "interval") \
  METRIC (saved_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  COUNTER (score_decisions, 0, PCNT_DECISIONS, "%", "decision") \
  COUNTER (searches, 2, CONF_INT, "", "interval") \
  METRIC (search_propagations, 2, PCNT_PROPS, "%", "propagations") \
  COUNTER (search_ticks, 2, PCNT_TICKS, "%", "ticks") \
  METRIC (sparse_garbage_collections, 2, PCNT_COLLECTIONS, "%", \
          "collections") \
  METRIC (stable_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  METRIC (stable_modes, 2, CONF_INT, "", "interval") \
  METRIC (stable_propagations, 1, PCNT_PROPS, "%", "propagations") \
  METRIC (stable_restarts, 1, PCNT_RESTARTS, "%", "restarts") \
  METRIC (stable_ticks, 2, PCNT_TICKS, "%", "ticks") \
  COUNTER (strengthened, 1, PCNT_SUBSUMPTION_CHECK, "%", "checks") \
  COUNTER (substituted, 1, PCNT_VARIABLES, "%", "variables") \
  COUNTER (substitute_ticks, 2, PCNT_TICKS, "%", "ticks") \
  COUNTER (subsumption_checks, 2, NO_SECONDARY, 0, 0) \
  STATISTIC (substitute_units, 1, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (substitutions, 2, CONF_INT, "", "interval") \
  COUNTER (subsumed, 1, PCNT_SUBSUMPTION_CHECK, "%", "checks") \
  COUNTER (sweep, 2, CONF_INT, "", "interval") \
  COUNTER (sweep_completed, 2, SWEEPS_PER_COMPLETED, "", "sweeps") \
  COUNTER (sweep_equivalences, 2, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (sweep_sat, 0, PCNT_SWEEP_SOLVED, "%", "sweep_solved") \
  COUNTER (sweep_solved, 2, PCNT_KITTEN_SOLVED, "%", "kitten_solved") \
  COUNTER (sweep_units, 2, PCNT_VARIABLES, "%", "variables") \
  STATISTIC (sweep_unsat, 0, PCNT_SWEEP_SOLVED, "%", "sweep_solved") \
  COUNTER (sweep_variables, 2, PCNT_VARIABLES, "%", "variables") \
  COUNTER (switched, 0, CONF_INT, "", "interval") \
  METRIC (target_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
  METRIC (target_saved, 1, CONF_INT, "", "interval") \
  STATISTIC (ticks, 2, PER_PROPAGATION, 0, "per prop") \
  METRIC (transitive_probes, 2, PER_VARIABLE, "", "per variable") \
  METRIC (transitive_propagations, 2, PCNT_PROPS, "%", "propagations") \
  METRIC (transitive_reduced, 1, PCNT_CLS_ADDED, "%", "added") \
  METRIC (transitive_reductions, 1, CONF_INT, "", "interval") \
  COUNTER (transitive_ticks, 2, PCNT_TICKS, "%", "ticks") \
  METRIC (transitive_units, 1, PCNT_VARIABLES, "%", "variables") \
  COUNTER (units, 2, PCNT_VARIABLES, "%", "variables") \
  COUNTER (variables_activated, 2, PER_VARIABLE, 0, "per variable") \
  COUNTER (variables_added, 2, PER_VARIABLE, 0, "per variable") \
  COUNTER (variables_removed, 2, PER_VARIABLE, 0, "variables") \
  METRIC (vectors_defrags_needed, 1, PCNT_DEFRAGS, "%", "defrags") \
  METRIC (vectors_enlarged, 2, CONF_INT, "", "interval") \
  COUNTER (vivifications, 2, CONF_INT, "", "interval") \
  COUNTER (vivified, 1, PCNT_VIVIFY_CHECK, "%", "checks") \
  COUNTER (vivify_checks, 0, PER_VIVIFICATION, "", "per vivify") \
  STATISTIC (vivify_probes, 1, PER_VIVIFY_CHECK, "", "checks") \
  STATISTIC (vivify_propagations, 2, PCNT_PROPS, "%", "propagations") \
  STATISTIC (vivify_reused, 1, PCNT_VIVIFY_PROBES, "%", "probes") \
  STATISTIC (vivify_ticks, 2, PCNT_TICKS, "%", "ticks") \
  COUNTER (vivify_units, 0, PCNT_VARIABLES, "%", "variables") \
  METRIC (walk_decisions, 1, PCNT_WALKS, "%", "walks") \
  COUNTER (walk_improved, 1, PCNT_WALKS, "%", "walks") \
  METRIC (walk_previous, 1, PCNT_WALKS, "%", "walks") \
  COUNTER (walks, 1, CONF_INT, "", "interval") \
  COUNTER (walk_steps, 2, PER_FLIPPED, 0, "per flipped") \
  COUNTER (warmups, 2, PCNT_WALKS, "%", "walks") \
  METRIC (weakened, 1, PCNT_CLS_ADDED, "%", "added")

/*------------------------------------------------------------------------*/
#ifdef METRICS

#define METRIC COUNTER
#define STATISTIC COUNTER

#ifndef STATISTICS
#define STATISTICS
#endif

#elif STATISTICS

#define METRIC IGNORE
#define STATISTIC COUNTER

#else

#define METRIC IGNORE
#define STATISTIC IGNORE

#endif
/*------------------------------------------------------------------------*/
// clang-format off

typedef struct statistics statistics;

struct statistics
{
#define COUNTER(NAME,VERBOSE,OTHER,UNITS,TYPE) \
  uint64_t NAME;
#define IGNORE(...)

  METRICS_COUNTERS_AND_STATISTICS

#undef COUNTER
#undef IGNORE
};

// clang-format on
/*------------------------------------------------------------------------*/

#define CLAUSES (IRREDUNDANT_CLAUSES + BINARY_CLAUSES + REDUNDANT_CLAUSES)
#define CONFLICTS (solver->statistics.conflicts)
#define DECISIONS (solver->statistics.decisions)
#define IRREDUNDANT_CLAUSES (solver->statistics.clauses_irredundant)
#define LEARNED_CLAUSES (solver->statistics.learned)
#define REDUNDANT_CLAUSES (solver->statistics.clauses_redundant)
#define BINARY_CLAUSES (solver->statistics.clauses_binary)
#define BINIRR_CLAUSES (BINARY_CLAUSES + IRREDUNDANT_CLAUSES)

/*------------------------------------------------------------------------*/

#define COUNTER(NAME, VERBOSE, OTHER, UNITS, TYPE) \
\
  static inline void kissat_inc_##NAME (statistics *statistics) { \
    assert (statistics->NAME < UINT64_MAX); \
    statistics->NAME++; \
  } \
\
  static inline void kissat_dec_##NAME (statistics *statistics) { \
    assert (statistics->NAME); \
    statistics->NAME--; \
  } \
\
  static inline void kissat_add_##NAME (statistics *statistics, \
                                        uint64_t n) { \
    assert (UINT64_MAX - n >= statistics->NAME); \
    statistics->NAME += n; \
  } \
\
  static inline void kissat_sub_##NAME (statistics *statistics, \
                                        uint64_t n) { \
    assert (n <= statistics->NAME); \
    statistics->NAME -= n; \
  } \
\
  static inline uint64_t kissat_get_##NAME (statistics *statistics) { \
    return statistics->NAME; \
  }

/*------------------------------------------------------------------------*/

#define IGNORE(NAME, VERBOSE, OTHER, UNITS, TYPE) \
\
  static inline void kissat_inc_##NAME (statistics *statistics) { \
    (void) statistics; \
  } \
\
  static inline void kissat_dec_##NAME (statistics *statistics) { \
    (void) statistics; \
  } \
\
  static inline void kissat_add_##NAME (statistics *statistics, \
                                        uint64_t n) { \
    (void) statistics; \
    (void) n; \
  } \
\
  static inline void kissat_sub_##NAME (statistics *statistics, \
                                        uint64_t n) { \
    (void) statistics; \
    (void) n; \
  } \
  static inline uint64_t kissat_get_##NAME (statistics *statistics) { \
    (void) statistics; \
    return UINT64_MAX; \
  }

/*------------------------------------------------------------------------*/
// clang-format off

METRICS_COUNTERS_AND_STATISTICS

#undef COUNTER
#undef IGNORE

// clang-format on
/*------------------------------------------------------------------------*/

#define INC(NAME) kissat_inc_##NAME (&solver->statistics)
#define DEC(NAME) kissat_dec_##NAME (&solver->statistics)
#define ADD(NAME, N) kissat_add_##NAME (&solver->statistics, (N))
#define SUB(NAME, N) kissat_sub_##NAME (&solver->statistics, (N))
#define GET(NAME) kissat_get_##NAME (&solver->statistics)

/*------------------------------------------------------------------------*/
#ifndef QUIET

struct kissat;

void kissat_statistics_print (struct kissat *, bool verbose);

// Format widths of individual parts during printing statistics lines.
// Shared between 'statistics.c' and 'resources.c' to align the printing.

#define SFW1 "28"
#define SFW2 "14"
#define SFW3 "5"
#define SFW4 "10"

#define SFW34 "16"         // SFE3 + space + SFE 4
#define SFW34EXTENDED "19" // SFE3 + space + SFE 4 + ".2"

#define PRINT_STAT(NAME, PRIMARY, SECONDARY, UNITS, TYPE) \
  do { \
    printf ("c %-" SFW1 "s %" SFW2 PRIu64 " ", NAME ":", \
            (uint64_t) PRIMARY); \
    const double SAVED_SECONDARY = (double) (SECONDARY); \
    const char *SAVED_UNITS = (const char *) (UNITS); \
    const char *SAVED_TYPE = (const char *) (TYPE); \
    if (SAVED_TYPE && SAVED_SECONDARY >= 0) { \
      if (SAVED_UNITS) \
        printf ("%" SFW34 ".0f %-2s", SAVED_SECONDARY, SAVED_UNITS); \
      else \
        printf ("%" SFW34EXTENDED ".2f", SAVED_SECONDARY); \
      fputc (' ', stdout); \
      fputs (SAVED_TYPE, stdout); \
    } \
    fputc ('\n', stdout); \
  } while (0)

#endif

#ifndef NDEBUG

struct kissat;
void kissat_check_statistics (struct kissat *);

#else

#define kissat_check_statistics(...) \
  do { \
  } while (0)

#endif

#endif
