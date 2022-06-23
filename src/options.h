#ifndef _options_h_INLCUDED
#define _options_h_INLCUDED

#include <assert.h>
#include <stdbool.h>

#define OPTIONS \
OPTION( ands, 1, 0, 1, "extract and eliminate and gates") \
OPTION( backbone, 1, 0, 2, "binary clause backbone (2=eager)") \
OPTION( backboneeffort, 20, 0, 1e5, "effort in per mille") \
OPTION( backbonemaxrounds, 1e3, 1, INT_MAX, "maximum backbone rounds") \
OPTION( backbonerounds, 100, 1, INT_MAX, "backbone rounds limit") \
OPTION( bump, 1, 0, 1, "enable variable bumping") \
OPTION( bumpreasons, 1, 0, 1, "bump reason side literals too") \
OPTION( bumpreasonslimit, 10, 1, INT_MAX, "relative reason literals limit") \
OPTION( bumpreasonsrate, 10, 1, INT_MAX, "decision rate limit") \
DBGOPT( check, 2, 0, 2, "check model (1) and derived clauses (2)") \
OPTION( chrono, 1, 0, 1, "allow chronological backtracking") \
OPTION( chronolevels, 100, 0, INT_MAX, "maximum jumped over levels") \
OPTION( compact, 1, 0, 1, "enable compacting garbage collection") \
OPTION( compactlim, 10, 0, 100, "compact inactive limit (in percent)") \
OPTION( decay, 50, 1, 200, "per mille scores decay") \
OPTION( definitioncores, 2, 1, 100, "how many cores") \
OPTION( definitions, 1, 0, 1, "extract general definitions") \
OPTION( definitionticks, 1e6, 0, INT_MAX, "kitten ticks limits") \
OPTION( defraglim, 75, 50, 100, "usable defragmentation limit in percent") \
OPTION( defragsize, 1<<18, 10, INT_MAX, "size defragmentation limit") \
OPTION( eliminate, 1, 0, 1, "bounded variable elimination (BVE)") \
OPTION( eliminatebound, 16 ,0 , 1<<13, "maximum elimination bound") \
OPTION( eliminateclslim, 100, 1, INT_MAX, "elimination clause size limit") \
OPTION( eliminateeffort, 100, 0, 2e3, "effort in per mille") \
OPTION( eliminateinit, 500, 0, INT_MAX, "initial elimination interval") \
OPTION( eliminateint, 500, 10, INT_MAX, "base elimination interval") \
OPTION( eliminateocclim, 2e3, 0, INT_MAX, "elimination occurrence limit") \
OPTION( eliminaterounds, 2, 1, 1e4, "elimination rounds limit") \
OPTION( emafast, 33, 10, 1e6, "fast exponential moving average window") \
OPTION( emaslow, 1e5, 100, 1e6, "slow exponential moving average window") \
EMBOPT( embedded, 1, 0, 1, "parse and apply embedded options") \
OPTION( equivalences, 1, 0, 1, "extract and eliminate equivalence gates") \
OPTION( extract, 1, 0, 1, "extract gates in variable elimination") \
OPTION( forcephase, 0, 0, 1, "force initial phase") \
OPTION( forward, 1, 0, 1, "forward subsumption in BVE") \
OPTION( forwardeffort, 100, 0, 1e6, "effort in per mille") \
OPTION( ifthenelse, 1, 0, 1, "extract and eliminate if-then-else gates") \
OPTION( incremental, 0, 0, 1, "enable incremental solving") \
LOGOPT( log, 0, 0, 5, "logging level (1=on,2=more,3=check,4/5=mem)") \
OPTION( mineffort, 10, 0, INT_MAX, "minimum absolute effort in millions") \
OPTION( minimize, 1, 0, 1, "learned clause minimization") \
OPTION( minimizedepth, 1e3, 1, 1e6, "minimization depth") \
OPTION( minimizeticks, 1, 0, 1, "count ticks in minimize and shrink") \
OPTION( modeinit, 1e3, 10, 1e8, "initial focused conflicts limit") \
OPTION( otfs, 1, 0, 1, "on-the-fly strengthening") \
OPTION( phase, 1, 0, 1, "initial decision phase") \
OPTION( phasesaving, 1, 0, 1, "enable phase saving") \
OPTION( probe, 1, 0, 1, "enable probing") \
OPTION( probeinit, 100, 0, INT_MAX, "initial probing interval") \
OPTION( probeint, 100, 2, INT_MAX, "probing interval") \
NQTOPT( profile, 2, 0, 4, "profile level") \
OPTION( promote, 1, 0, 1, "promote clauses") \
NQTOPT( quiet, 0, 0, 1, "disable all messages") \
OPTION( reduce, 1, 0, 1, "learned clause reduction") \
OPTION( reducefraction, 75, 10, 100, "reduce fraction in percent") \
OPTION( reduceinit, 1e3, 2, 1e5, "initial reduce interval") \
OPTION( reduceint, 1e3, 2, 1e5, "base reduce interval") \
OPTION( reluctant, 1, 0, 1, "stable reluctant doubling restarting") \
OPTION( reluctantint, 1<<10, 2, 1<<15, "reluctant interval") \
OPTION( reluctantlim, 1<<20, 0, 1<<30, "reluctant limit (0=unlimited)") \
OPTION( rephase, 1, 0, 1, "reinitialization of decision phases") \
OPTION( rephaseinit, 1e3, 10, 1e5, "initial rephase interval") \
OPTION( rephaseint, 1e3, 10, 1e5, "base rephase interval") \
OPTION( restart, 1, 0, 1, "enable restarts") \
OPTION( restartint, RESTARTINT_DEFAULT, 1, 1e4, "base restart interval") \
OPTION( restartmargin, 10, 0, 25, "fast/slow margin in percent") \
OPTION( seed, 0, 0, INT_MAX, "random seed") \
OPTION( shrink, 3, 0, 3, "learned clauses (1=bin,2=lrg,3=rec)") \
OPTION( simplify, 1, 0, 1, "enable probing and elimination") \
OPTION( stable, STABLE_DEFAULT, 0, 2, "enable stable search mode") \
NQTOPT( statistics, 0, 0, 1, "print complete statistics") \
OPTION( substitute, 1, 0, 1, "equivalent literal substitution") \
OPTION( substituteeffort, 10, 1, 1e3, "effort in per mille") \
OPTION( substituterounds, 2, 1, 100, "maximum substitution rounds") \
OPTION( subsumeclslim, 1e3, 1, INT_MAX, "subsumption clause size limit") \
OPTION( subsumeocclim, 1e3, 0, INT_MAX, "subsumption occurrence limit") \
OPTION( sweep, 1, 0, 1, "enable SAT sweeping") \
OPTION( sweepclauses, 1024, 0, INT_MAX, "environment clauses") \
OPTION( sweepdepth, 1, 0, INT_MAX, "environment depth") \
OPTION( sweepeffort, 10, 0, 1e4, "effort in per mille") \
OPTION( sweepfliprounds, 1, 0, INT_MAX, "flipping rounds") \
OPTION( sweepmaxclauses, 4096, 2, INT_MAX, "maximum environment clauses") \
OPTION( sweepmaxdepth, 2, 1, INT_MAX, "maximum environment depth") \
OPTION( sweepmaxvars, 128, 2, INT_MAX, "maximum environment variables") \
OPTION( sweepvars, 128, 0, INT_MAX, "environment variables") \
OPTION( target, TARGET_DEFAULT, 0, 2, "target phases (1=stable,2=focused)") \
OPTION( tier1, 2, 1, 100, "learned clause tier one glue limit") \
OPTION( tier2, 6, 1,1e3, "learned clause tier two glue limit") \
OPTION( tumble, 1, 0, 1, "tumbled external indices order") \
NQTOPT( verbose, 0, 0, 3, "verbosity level") \
OPTION( vivify, 1, 0, 1, "vivify clauses") \
OPTION( vivifyeffort, 100, 0, 1e3, "effort in per mille") \
OPTION( vivifyirred, 1, 1, 100, "relative irredundant effort") \
OPTION( vivifytier1, 3, 1, 100, "relative tier1 effort") \
OPTION( vivifytier2, 6, 1, 100, "relative tier2 effort") \
OPTION( walkeffort, 50, 0, 1e6, "effort in per mille") \
OPTION( walkinitially, 0, 0, 1, "initial local search") \
OPTION( warmup, 1, 0, 1, "initialize phases by unit propagation") \

// *INDENT-OFF*

#define TARGET_SAT 2
#define TARGET_DEFAULT 1

#define STABLE_DEFAULT 1
#define STABLE_UNSAT 0

#define RESTARTINT_DEFAULT 1
#define RESTARTINT_SAT 50

#ifdef SAT
#undef TARGET_DEFAULT
#define TARGET_DEFAULT TARGET_SAT
#undef RESTARTINT_DEFAULT
#define RESTARTINT_DEFAULT RESTARTINT_SAT
#endif

#ifdef UNSAT
#undef STABLE_DEFAULT
#define STABLE_DEFAULT STABLE_UNSAT
#endif

#if defined(LOGGING) && !defined(QUIET)
#define LOGOPT OPTION
#else
#define LOGOPT(...) /**/
#endif

#ifndef QUIET
#define NQTOPT OPTION
#else
#define NQTOPT(...) /**/
#endif

#ifndef NDEBUG
#define DBGOPT OPTION
#else
#define DBGOPT(...) /**/
#endif

#ifdef EMBEDDED
#define EMBOPT OPTION
#else
#define EMBOPT(...) /**/
#endif

// *INDENT-ON*

typedef struct opt opt;

struct opt
{
  const char *name;
#ifndef NOPTIONS
  int value;
  const int low;
  const int high;
#else
  const int value;
#endif
  const char *description;
};

extern const opt *kissat_options_begin;
extern const opt *kissat_options_end;

#define all_options(O) \
  opt const * O = kissat_options_begin; O != kissat_options_end; ++O

const char *kissat_parse_option_name (const char *arg, const char *name);
bool kissat_parse_option_value (const char *val_str, int *res_ptr);

#ifndef NOPTIONS

void kissat_options_usage (void);

const opt *kissat_options_has (const char *name);

#define kissat_options_max_name_buffer_size ((size_t) 32)

bool kissat_options_parse_arg (const char *arg, char *name, int *val_str);
void kissat_options_print_value (int value, char *buffer);

typedef struct options options;

struct options
{
#define OPTION(N,V,L,H,D) int N;
  OPTIONS
#undef OPTION
};

void kissat_init_options (options *);

int kissat_options_get (const options *, const char *name);
int kissat_options_set_opt (options *, const opt *, int new_value);
int kissat_options_set (options *, const char *name, int new_value);

void kissat_print_embedded_option_list (void);
void kissat_print_option_range_list (void);

static inline int *
kissat_options_ref (const options * options, const opt * o)
{
  if (!o)
    return 0;
  assert (kissat_options_begin <= o);
  assert (o < kissat_options_end);
  return (int *) options + (o - kissat_options_begin);
}

#define GET_OPTION(NAME) ((int) solver->options.NAME)

#else

void kissat_init_options (void);
int kissat_options_get (const char *name);

#define GET_OPTION(N) kissat_options_ ## N

#define OPTION(N,V,L,H,D) static const int GET_OPTION(N) = (int)(V);
OPTIONS
#undef OPTION
#endif
#define GET1K_OPTION(NAME) (((int64_t) 1000) * GET_OPTION (NAME))
#endif
