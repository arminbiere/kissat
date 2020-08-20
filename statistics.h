#ifndef _statistics_h_INCLUDED
#define _statistics_h_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#define STATISTICS \
\
METRIC( allocated_collected, 2, PCNT_RESIDENT_SET, "%", "resident set") \
METRIC( allocated_current, 2, PCNT_RESIDENT_SET, "%", "resident set") \
METRIC( allocated_max, 2, PCNT_RESIDENT_SET, "%", "resident set") \
METRIC( ands_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
METRIC( arena_enlarged, 1, PCNT_ARENA_RESIZED, "%", "resize") \
METRIC( arena_garbage, 1, PCNT_RESIDENT_SET, "%", "resident set") \
METRIC( arena_resized, 1, CONF_INT, "", "interval") \
METRIC( arena_shrunken, 1, PCNT_ARENA_RESIZED, "%", "resize") \
STATISTIC( autarky, 1, PCNT_VARIABLES, "%", "variables") \
METRIC( autarky_determined, 1, CONF_INT, "", "interval") \
METRIC( backward_strengthened, 1, PCNT_STR, "%", "strengthened") \
METRIC( backward_subsumed, 1, PCNT_SUB, "%", "subsumed") \
METRIC( chronological, 1, PCNT_CONFLICTS, "%", "conflicts") \
METRIC( chronological_reused_trails, 1, PCNT_CONFLICTS, "%", "conflicts") \
METRIC( clauses_added, 2, PCNT_CLS_ADDED, "%", "added") \
METRIC( clauses_deleted, 2, PCNT_CLS_ADDED, "%", "added") \
METRIC( clauses_improved, 2, PCNT_CLS_ADDED, "%", "added") \
COUNTER( clauses_irredundant, 2, NO_SECONDARY, 0, 0) \
METRIC( clauses_kept2, 2, PCNT_CLS_ADDED, "%", "added") \
METRIC( clauses_kept3, 2, PCNT_CLS_ADDED, "%", "added") \
METRIC( clauses_original, 2, PCNT_CLS_ADDED, "%", "added") \
METRIC( clauses_promoted1, 2, PCNT_CLS_ADDED, "%", "added") \
METRIC( clauses_promoted2, 2, PCNT_CLS_ADDED, "%", "added") \
METRIC( clauses_reduced, 2, PCNT_CLS_ADDED, "%", "added") \
COUNTER( clauses_redundant, 2, NO_SECONDARY, 0, 0) \
METRIC( compacted, 1, PCNT_REDUCTIONS, "%", "reductions") \
COUNTER( conflicts, 0, PER_SECOND, 0, "per second") \
COUNTER( decisions, 0, PER_CONFLICT, 0, "per conflict") \
METRIC( defragmentations, 1, CONF_INT, "", "interval") \
METRIC( dense_garbage_collections, 2, PCNT_COLLECTIONS, "%", "collections") \
METRIC( dense_propagations, 1, PCNT_PROPS, "%", "propagations") \
METRIC( dense_ticks, 1, PCNT_TICKS, "%", "ticks") \
METRIC( duplicated, 1, PCNT_CLS_ADDED, "%", "added") \
METRIC( eager_subsumed, 1, PCNT_SUB, "%", "subsumed") \
STATISTIC( eliminated, 1, PCNT_VARIABLES, "%", "variables") \
COUNTER( eliminations, 2, CONF_INT, "", "interval") \
METRIC( equivalences_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
METRIC( extensions, 1, PCNT_SEARCHES, "%", "searches") \
METRIC( failed, 2, PCNT_VARIABLES, "%", "variables") \
METRIC( failed_literal_probings, 1, CONF_INT, "", "interval") \
STATISTIC( flipped, 1, PER_WALKS, 0, "per walk") \
METRIC( focused_modes, 1, CONF_INT, "", "interval") \
METRIC( focused_propagations, 1, PCNT_PROPS, "%", "propagations") \
METRIC( focused_ticks, 1, PCNT_TICKS, "%", "ticks") \
METRIC( forward_strengthened, 1, PCNT_STR, "%", "strengthened") \
METRIC( forward_subsumed, 1, PCNT_SUB, "%", "subsumed") \
METRIC( forward_subsumptions, 1, CONF_INT, "", "interval") \
METRIC( garbage_collections, 2, CONF_INT, "", "interval") \
STATISTIC( gates, 1, PCNT_ELIMINATED, "%", "eliminated") \
METRIC( hyper_binaries, 2, PCNT_REDUNDANT_CLAUSES, "%", "redundant") \
METRIC( hyper_binary_resolved, 1, NO_SECONDARY, 0, 0) \
METRIC( hyper_ternaries, 2, PCNT_REDUNDANT_CLAUSES, "%", "redundant") \
METRIC( hyper_ternary2_resolved, 2, PCNT_TRN_RESOLVED, "%", "resolved") \
METRIC( hyper_ternary3_resolved, 2, PCNT_TRN_RESOLVED, "%", "resolved") \
METRIC( hyper_ternary_phases, 1, CONF_INT, "", "interval") \
METRIC( hyper_ternary_resolutions, 2, PCNT_TRN_RESOLVED, "", "per resolved") \
METRIC( hyper_ternary_resolved, 1, NO_SECONDARY, 0, 0) \
COUNTER( hyper_ternary_steps, 2, PER_TRN_RESOLVED, 0, "resolved") \
METRIC( if_then_else_eliminated, 1, PCNT_ELIMINATED, "%", "eliminated") \
METRIC( initial_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
STATISTIC( learned, 1, PCNT_CONFLICTS, "%", "conflicts") \
METRIC( literals_deduced, 2, PER_CLS_LEARNED, 0, "per clause") \
METRIC( literals_flushed, 2, PER_FIXED, 0, "per fixed") \
METRIC( literals_learned, 2, PER_CLS_LEARNED, 0, "per clause") \
METRIC( minimized, 1, PCNT_LITERALS_DEDUCED, "%", "deduced") \
METRIC( moved, 1, PCNT_REDUCTIONS, "%", "reductions") \
METRIC( on_the_fly_strengthened, 1, PCNT_CONFLICTS, "%", "of conflicts") \
METRIC( on_the_fly_subsumed, 1, PCNT_CONFLICTS, "%", "of conflicts") \
METRIC( probing_propagations, 1, PCNT_PROPS, "%", "propagations") \
COUNTER( probings, 2, CONF_INT, "", "interval") \
COUNTER( probing_ticks, 2, PCNT_TICKS, "%", "ticks") \
COUNTER( propagations, 0, PER_SECOND, "", "per second") \
COUNTER( reductions, 1, CONF_INT, "", "interval") \
COUNTER( rephased, 1, CONF_INT, "", "interval") \
METRIC( rescaled, 2, CONF_INT, "", "interval") \
COUNTER( resolutions, 2, PER_SECOND, 0, "per second") \
COUNTER( restarts, 1, CONF_INT, 0, "interval") \
METRIC( restarts_reused_trails, 1, PCNT_RESTARTS, "%", "restarts") \
METRIC( reused_levels, 2, PER_REUSED_TRAIL, 0, "per reused") \
METRIC( saved_decisions, 0, PCNT_DECISIONS, "%", "decisions") \
COUNTER( searches, 2, CONF_INT, "", "interval") \
METRIC( search_propagations, 2, PCNT_PROPS, "%", "propagations") \
COUNTER( search_ticks, 2, PCNT_TICKS, "%", "ticks") \
METRIC( sparse_garbage_collections, 2, PCNT_COLLECTIONS, "%", "collections") \
COUNTER( stable_modes, 2, CONF_INT, "", "interval") \
METRIC( stable_propagations, 1, PCNT_PROPS, "%", "propagations") \
METRIC( stable_ticks, 2, PCNT_TICKS, "%", "ticks") \
METRIC( strengthened, 1, PCNT_SUBSUMPTION_CHECK, "%", "checks") \
STATISTIC( substituted, 1, PCNT_VARIABLES, "%", "variables") \
STATISTIC( substitutions, 2, CONF_INT, "", "interval") \
METRIC( subsumed, 1, PCNT_SUBSUMPTION_CHECK, "%", "checks") \
COUNTER( subsumption_checks, 2, NO_SECONDARY, 0, 0) \
STATISTIC( switched_modes, 2, CONF_INT, "", "interval") \
METRIC( target_decisions, 1, PCNT_DECISIONS, "%", "decisions") \
STATISTIC( ticks, 2, PER_PROPAGATION, 0, "per prop") \
METRIC( transitive_propagations, 2, PCNT_PROPS, "%", "propagations") \
METRIC( transitive_reduced, 1, PCNT_CLS_ADDED, "%", "added") \
COUNTER( transitive_ticks, 2, PCNT_TICKS, "%", "ticks") \
COUNTER( units, 2, PCNT_VARIABLES, "%", "variables") \
COUNTER( variables_added, 2, PER_VARIABLE, 0, "per variable") \
COUNTER( variables_removed, 2, PER_VARIABLE, 0, "variables") \
METRIC( vectors_defrags_needed, 1, PCNT_DEFRAGS, "%", "defrags") \
METRIC( vectors_enlarged, 2, CONF_INT, "", "interval") \
STATISTIC( vivification_checks, 2, NO_SECONDARY, 0, 0) \
METRIC( vivified, 1, PCNT_VIVIFICATION_CHECK, "%", "checks") \
METRIC( vivify_assumed, 1, PCNT_VIVIFY_PROBES, "%", "probes") \
METRIC( vivify_implied, 1, PCNT_VIVIFIED, "%", "vivified") \
METRIC( vivify_irredundant, 1, PCNT_VIVIFIED, "%", "vivified") \
METRIC( vivify_probes, 1, PER_VIVIFICATION_CHECK, "", "checks") \
METRIC( vivify_redundant, 1, PCNT_VIVIFIED, "%", "vivified") \
METRIC( vivify_reused, 1, PCNT_VIVIFY_PROBES, "%", "probes") \
METRIC( vivify_strengthened, 1, PCNT_STR, "%", "strengthened") \
METRIC( vivify_str_irr, 1, PCNT_VIVIFY_STR, "%", "strengthened") \
METRIC( vivify_str_red, 1, PCNT_VIVIFY_STR, "%", "strengthened") \
METRIC( vivify_sub_irr, 1, PCNT_VIVIFY_SUB, "%", "subsumed") \
METRIC( vivify_sub_red, 1, PCNT_VIVIFY_SUB, "%", "subsumed") \
METRIC( vivify_subsumed, 1, PCNT_SUB, "%", "subsumed") \
METRIC( walk_improved, 1, PCNT_WALKS, "%", "walks") \
STATISTIC( walks, 1, CONF_INT, "", "interval") \
COUNTER( walk_steps, 2, PER_FLIPPED, 0, "per flipped") \
METRIC( weakened, 1, PCNT_CLS_ADDED, "%", "added") \
METRIC( xors_eliminated, 0, PCNT_ELIMINATED, "%", "eliminated") \

/*------------------------------------------------------------------------*/
#ifdef NSTATISTICS

#define METRIC IGNORE
#define STATISTIC IGNORE

#elif NMETRICS

#define METRIC IGNORE
#define STATISTIC COUNTER

#else

#define METRIC COUNTER
#define STATISTIC COUNTER

#endif
/*------------------------------------------------------------------------*/
// *INDENT-OFF*

typedef struct statistics statistics;

struct statistics
{
#define COUNTER(NAME,VERBOSE,OTHER,UNITS,TYPE) \
  uint64_t NAME;
#define IGNORE(...)

  STATISTICS

#undef COUNTER
#undef IGNORE
};

// *INDENT-ON*
/*------------------------------------------------------------------------*/

#define CLAUSES (IRREDUNDANT_CLAUSES + REDUNDANT_CLAUSES)
#define CONFLICTS (solver->statistics.conflicts)
#define IRREDUNDANT_CLAUSES (solver->statistics.clauses_irredundant)
#define LEARNED_CLAUSES (solver->statistics.learned)
#define REDUNDANT_CLAUSES (solver->statistics.clauses_redundant)

/*------------------------------------------------------------------------*/

#define COUNTER(NAME,VERBOSE,OTHER,UNITS,TYPE) \
\
static inline void \
kissat_inc_ ## NAME (statistics * statistics) \
{ \
  assert (statistics->NAME < UINT64_MAX); \
  statistics->NAME++; \
} \
\
static inline void \
kissat_dec_ ## NAME (statistics * statistics) \
{ \
  assert (statistics->NAME); \
  statistics->NAME--; \
} \
\
static inline void \
kissat_add_ ## NAME (statistics * statistics, uint64_t n) \
{ \
  assert (UINT64_MAX - n >= statistics->NAME); \
  statistics->NAME += n; \
} \
\
static inline void \
kissat_sub_ ## NAME (statistics * statistics, uint64_t n) \
{ \
  assert (n <= statistics->NAME); \
  statistics->NAME -= n; \
} \
\
static inline uint64_t \
kissat_get_ ## NAME (statistics * statistics) \
{ \
  return statistics->NAME; \
}

/*------------------------------------------------------------------------*/

#define IGNORE(NAME,VERBOSE,OTHER,UNITS,TYPE) \
\
static inline void \
kissat_inc_ ## NAME (statistics * statistics) \
{ \
  (void) statistics; \
} \
\
static inline void \
kissat_dec_ ## NAME (statistics * statistics) \
{ \
  (void) statistics; \
} \
\
static inline void \
kissat_add_ ## NAME (statistics * statistics, uint64_t n) \
{ \
  (void) statistics; \
  (void) n; \
} \
\
static inline void \
kissat_sub_ ## NAME (statistics * statistics, uint64_t n) \
{ \
  (void) statistics; \
  (void) n; \
} \
static inline uint64_t \
kissat_get_ ## NAME (statistics * statistics) \
{ \
  (void) statistics; \
  return UINT64_MAX; \
}

/*------------------------------------------------------------------------*/
// *INDENT-OFF*

STATISTICS

#undef COUNTER
#undef IGNORE

// *INDENT-ON*
/*------------------------------------------------------------------------*/

#define INC(NAME) kissat_inc_ ## NAME (&solver->statistics)
#define DEC(NAME) kissat_dec_ ## NAME (&solver->statistics)
#define ADD(NAME,N) kissat_add_ ## NAME (&solver->statistics, (N))
#define SUB(NAME,N) kissat_sub_ ## NAME (&solver->statistics, (N))
#define GET(NAME) kissat_get_ ## NAME (&solver->statistics)

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

#define SFW34 "16"		// SFE3 + space + SFE 4
#define SFW34EXTENDED  "19"	// SFE3 + space + SFE 4 + ".2"

#define PRINT_STAT(NAME,PRIMARY,SECONDARY,UNITS,TYPE) \
do { \
  printf ("c %-" SFW1 "s %"  SFW2 PRIu64 " ", NAME ":", (uint64_t) PRIMARY); \
  const double SAVED_SECONDARY = (double) (SECONDARY); \
  const char * SAVED_UNITS = (const char*) (UNITS); \
  const char * SAVED_TYPE = (const char *) (TYPE); \
  if (SAVED_TYPE && SAVED_SECONDARY >= 0) \
    { \
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

#define kissat_check_statistics(...) do { } while (0)

#endif

#endif
