#ifndef _options_h_INLCUDED
#define _options_h_INLCUDED

#include <assert.h>
#include <stdbool.h>

#define OPTIONS \
OPTION( ands, 1, 0, 1, "extract and eliminate and gates") \
OPTION( autarky, 1, 0, 1, "delay autarky reasoning") \
OPTION( autarkydelay, 1, 0, 1, "enable autarky reasoning") \
OPTION( backward, 1, 0, 1, "backward subsumption in BVE") \
OPTION( bumpreasons, 1, 0, 1, "bump reason side literals too") \
DBGOPT( check, 2, 0, 2, "check model (1) and derived clauses (2)") \
OPTION( chrono, 1, 0, 1, "allow chronological backtracking") \
OPTION( chronolevels, 100, 0, INT_MAX, "maximum jumped over levels") \
OPTION( compact, 1, 0, 1, "enable compacting garbage collection") \
OPTION( compactlim, 10, 0, 100, "compact inactive limit (in percent)") \
OPTION( decay, 50, 1, 200, "per mille scores decay") \
OPTION( defraglim, 75, 50, 100, "usable defragmentation limit in percent") \
OPTION( defragsize, 1<<18, 10, INT_MAX, "size defragmentation limit") \
OPTION( delay, 2, 0, 10, "maximum delay (autarky, failed, ...)") \
OPTION( eagersubsume, 20, 0, 100, "eagerly subsume recently learned clauses") \
OPTION( eliminate, 1, 0, 1, "bounded variable elimination (BVE)") \
OPTION( eliminatebound, 16 ,0 , 1<<13, "maximum elimination bound") \
OPTION( eliminateclslim, 1e2, 1, INT_MAX, "elimination clause size limit") \
OPTION( eliminatedelay, 0, 0, 1, "delay variable elimination") \
OPTION( eliminateinit, 500, 0, INT_MAX, "initial elimination interval") \
OPTION( eliminateint, 500, 10, INT_MAX, "base elimination interval") \
OPTION( eliminatemaxeff, 100, 1, 1e6, "maximum relative efficiency") \
OPTION( eliminatemineff, 1e6, 0, 1e8, "minimum elimination efficiency") \
OPTION( eliminateocclim, 1e3, 0, INT_MAX, "elimination occurrence limit") \
OPTION( eliminatereleff, 1e2, 0, 2e3, "relative efficiency in per mille") \
OPTION( eliminaterounds, 2, 1, 100, "elimination rounds limit") \
OPTION( emafast, 33, 10, 1e6, "fast exponential moving average window") \
OPTION( emaslow, 1e5, 100, 1e6, "slow exponential moving average window") \
EMBOPT( embedded, 1, 0, 1, "parse and apply embedded options") \
OPTION( equivalences, 1, 0, 1, "extract and eliminate equivalence gates") \
OPTION( extract, 1, 0, 1, "extract gates in variable elimination") \
OPTION( failed, 1, 0, 1, "failed literal probing") \
OPTION( faileddelay, 1, 0, 1, "delay failed literal probing") \
OPTION( failedmaxeff, 100, 1, 1e5, "maximum relative efficiency") \
OPTION( failedmineff, 5e5, 0, 1e8, "minimum probe efficiency") \
OPTION( failedreleff, 2, 0, 1e3, "relative efficiency in per mille") \
OPTION( failedrounds, 2, 1, 100, "failed literal probing rounds") \
OPTION( forward, 1, 0, 1, "forward subsumption in BVE") \
OPTION( hyper, 1, 0, 1, "on-the-fly hyper binary resolution") \
OPTION( ifthenelse, 1, 0, 1, "extract and eliminate if-then-else gates") \
OPTION( incremental, 0, 0, 1, "enable incremental solving") \
LOGOPT( log, 0, 0, 5, "logging level (1=on,2=more,3=check,4/5=mem)") \
OPTION( minimizedepth, 1e3, 1, 1e6, "minimization depth") \
OPTION( modeinit, 1e3, 10, 1e8, "initial mode change interval") \
OPTION( modeint, 1e3, 10, 1e8, "base mode change interval") \
OPTION( otfs, 1, 0, 1, "on-the-fly strengthening") \
OPTION( phase, 1, 0, 1, "initial decision phase") \
OPTION( probe, 1, 0, 1, "enable probing") \
OPTION( probedelay, 0, 0, 1, "delay probing") \
OPTION( probeinit, 100, 0, INT_MAX, "initial probing interval") \
OPTION( probeint, 100, 2, INT_MAX, "probing interval") \
NQTOPT( profile, 2, 0, 4, "profile level") \
NQTOPT( quiet, 0, 0, 1, "disable all messages") \
OPTION( really, 1, 0, 1, "delay preprocessing after scheduling") \
OPTION( reduce, 1, 0, 1, "learned clause reduction") \
OPTION( reducefraction, 75, 10, 100, "reduce fraction in percent") \
OPTION( reduceinit, 3e2, 2, 1e5, "initial reduce interval") \
OPTION( reduceint, 3e2, 2, 1e5, "base reduce interval") \
OPTION( reducerestart, 0, 0, 2, "restart at reduce (1=stable,2=always)") \
OPTION( reluctant, 1, 0, 1, "stable reluctant doubling restarting") \
OPTION( reluctantint, 1<<10, 2, 1<<15, "reluctant interval") \
OPTION( reluctantlim, 1<<20, 0, 1<<30, "reluctant limit (0=unlimited)") \
OPTION( rephase, 1, 0, 1, "reinitialization of decision phases") \
OPTION( rephaseinit, 1e3, 10, 1e5, "initial rephase interval") \
OPTION( rephaseint, 1e3, 10, 1e5, "base rephase interval") \
OPTION( restart, 1, 0, 1, "enable restarts") \
OPTION( restartint, 1, 1, 1e4, "base restart interval") \
OPTION( restartmargin, 10, 0, 25, "fast/slow margin in percent") \
OPTION( seed, 0, 0, INT_MAX, "random seed") \
OPTION( simplify, 1, 0, 1, "enable probing and elimination") \
OPTION( stable, STABLE_DEFAULT, 0, 2, "enable stable search mode") \
OPTION( stablebias, 100, 0, 1e3, "stable bias in percent") \
NQTOPT( statistics, 0, 0, 1, "print complete statistics") \
OPTION( substitute, 1, 0, 1, "equivalent literal substitution") \
OPTION( substitutelim, 1e7, 0, INT_MAX, "substitute multiple round limit") \
OPTION( substitutemineff, 1e6, 0, INT_MAX, "minimum efficiency") \
OPTION( substituterounds, 2, 1, 100, "maximum substitution rounds") \
OPTION( subsumeclslim, 1e3, 1, INT_MAX, "subsumption clause size limit") \
OPTION( subsumemaxeff, 100, 1, 1e6, "maximum relative efficiency") \
OPTION( subsumemineff, 1e6, 0, 1e8, "minimum subsume efficiency") \
OPTION( subsumeocclim, 1e3, 0, INT_MAX, "subsumption occurrence limit") \
OPTION( subsumereleff, 1e4, 0, 1e6, "relative efficiency in per mille") \
OPTION( target, TARGET_DEFAULT, 0, 2, "target phases (1=stable,2=focused)") \
OPTION( ternary, 1, 0, 1, "enable hyper ternary resolution") \
OPTION( ternarydelay, 1, 0, 1, "delay hyper ternary resolution") \
OPTION( ternarymaxadd, 20, 0, 1e4, "maximum clauses added in percent") \
OPTION( ternarymaxeff, 100, 1, 1e4, "maximum relative efficiency") \
OPTION( ternarymineff, 1e6, 0, 1e8, "minimum ternary efficiency") \
OPTION( ternaryreleff, 40, 0, 2e3, "relative efficiency in per mille") \
OPTION( tier1, 2, 1,1e2, "learned clause tier one glue limit") \
OPTION( tier2, 6, 1,1e3, "learned clause tier two glue limit") \
OPTION( transitive, 1, 0, 1, "transitive reduction of binary clauses") \
OPTION( transitivemaxeff, 100, 1, 1e4, "maximum relative efficiency") \
OPTION( transitivemineff, 1e6, 0, 1e8, "minimum transitive efficiency") \
OPTION( transitivereleff, 20, 0, 2e3, "relative efficiency in per mille") \
OPTION( tumble, 1, 0, 1, "tumbled external indices order") \
NQTOPT( verbose, 0, 0, 3, "verbosity level") \
OPTION( vivify, 1, 0, 1, "vivify clauses") \
OPTION( vivifyimply, 2, 0, 2, "remove implied redundant clauses") \
OPTION( vivifymaxeff, 50, 1, 1e6, "maximum relative efficiency") \
OPTION( vivifymineff, 2e5, 0, 1e8, "minimum vivify efficiency") \
OPTION( vivifyreleff, 2, 0, 1e3, "relative efficiency in per mille") \
OPTION( walkinitially, 0, 0, 1, "initial local search") \
OPTION( walkmaxeff, 100, 1, INT_MAX, "maximum relative efficiency") \
OPTION( walkmineff, 1e7, 0, INT_MAX, "minimum vivify efficiency") \
OPTION( walkreleff, 10, 0, 1e3, "relative efficiency in per mille") \
OPTION( walkrounds, 1, 1, 1e5, "rounds per walking phase") \
OPTION( xors, 1, 0, 1, "extract and eliminate XOR gates") \
OPTION( xorsbound, 1 ,0 , 1<<13, "minimum elimination bound") \
OPTION( xorsclslim, 5, 3, 31, "XOR extraction clause size limit") \

// *INDENT-OFF*

#define TARGET_SAT 2
#define TARGET_DEFAULT 1

#define STABLE_DEFAULT 1
#define STABLE_UNSAT 0

#ifdef SAT
#undef TARGET_DEFAULT
#define TARGET_DEFAULT TARGET_SAT
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

#ifndef NEMBEDDED
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
  const opt * O = kissat_options_begin; O != kissat_options_end; ++O

const char *kissat_parse_option_name (const char *arg, const char *name);
bool kissat_parse_option_value (const char *val_str, int *res_ptr);

#ifndef NOPTIONS

void kissat_options_usage (void);

const opt *kissat_options_has (const char *name);

#define kissat_options_max_name_buffer_size 32

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
#endif
