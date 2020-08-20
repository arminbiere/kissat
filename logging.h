#ifndef _logging_h_INCLUDED
#define _logging_h_INCLUDED

#if defined(LOGGING) && !defined(QUIET)

#include "extend.h"
#include "reference.h"
#include "watch.h"

void kissat_log_msg (kissat *, const char *fmt, ...);

const char *kissat_log_lit (kissat *, unsigned lit);

void kissat_log_clause (kissat *, clause *, const char *, ...);
void kissat_log_binary (kissat *, unsigned, unsigned, const char *, ...);
void kissat_log_unary (kissat *, unsigned, const char *, ...);

void kissat_log_lits (kissat *, size_t, const unsigned *, const char *, ...);
void kissat_log_ints (kissat *, size_t, const int *, const char *, ...);
void kissat_log_unsigneds (kissat *, size_t,
			   const unsigned *, const char *, ...);

void kissat_log_extensions (kissat *,
			    size_t, const extension *, const char *, ...);

void kissat_log_xor (struct kissat *, unsigned lit,
		     unsigned size, unsigned *lits, const char *fmt, ...);

void kissat_log_ref (kissat *, reference, const char *, ...);
void kissat_log_resolvent (kissat *, const char *, ...);

void kissat_log_watch (kissat *, unsigned, watch, const char *, ...);

#define LOG(...) \
do { \
  if (solver && GET_OPTION (log)) \
    kissat_log_msg (solver, __VA_ARGS__); \
} while (0)

#define LOG2(...) \
do { \
  if (solver && GET_OPTION (log) > 1) \
    kissat_log_msg (solver, __VA_ARGS__); \
} while (0)

#define LOG3(...) \
do { \
  if (solver && GET_OPTION (log) > 2) \
    kissat_log_msg (solver, __VA_ARGS__); \
} while (0)

#define LOG4(...) \
do { \
  if (solver && GET_OPTION (log) > 3) \
    kissat_log_msg (solver, __VA_ARGS__); \
} while (0)

#define LOG5(...) \
do { \
  if (solver && GET_OPTION (log) > 4) \
    kissat_log_msg (solver, __VA_ARGS__); \
} while (0)

#define LOGLITS(...) \
do { \
  if (solver && GET_OPTION (log)) \
    kissat_log_lits (solver, __VA_ARGS__); \
} while (0)

#define LOGLITS3(...) \
do { \
  if (GET_OPTION (log) > 2) \
    kissat_log_lits (solver, __VA_ARGS__); \
} while (0)

#define LOGRES(...) \
do { \
  if (solver && GET_OPTION (log)) \
    kissat_log_resolvent (solver, __VA_ARGS__); \
} while (0)

#define LOGRES2(...) \
do { \
  if (solver && GET_OPTION (log) > 1) \
    kissat_log_resolvent (solver, __VA_ARGS__); \
} while (0)

#define LOGEXT(...) \
do { \
  if (GET_OPTION (log)) \
    kissat_log_extensions (solver, __VA_ARGS__); \
} while (0)

#define LOGEXT2(...) \
do { \
  if (GET_OPTION (log) > 1) \
    kissat_log_extensions (solver, __VA_ARGS__); \
} while (0)

#define LOGINTS(...) \
do { \
  if (solver && GET_OPTION (log)) \
    kissat_log_ints (solver, __VA_ARGS__); \
} while (0)

#define LOGINTS3(...) \
do { \
  if (GET_OPTION (log) > 2) \
    kissat_log_ints (solver, __VA_ARGS__); \
} while (0)

#define LOGUNSIGNEDS3(...) \
do { \
  if (solver && GET_OPTION (log) > 2) \
    kissat_log_unsigneds (solver, __VA_ARGS__); \
} while (0)

#define LOGCLS(...) \
do { \
  if (solver && GET_OPTION (log)) \
    kissat_log_clause (solver, __VA_ARGS__); \
} while (0)

#define LOGCLS2(...) \
do { \
  if (GET_OPTION (log) > 1) \
    kissat_log_clause (solver, __VA_ARGS__); \
} while (0)

#define LOGCLS3(...) \
do { \
  if (GET_OPTION (log) > 2) \
    kissat_log_clause (solver, __VA_ARGS__); \
} while (0)

#define LOGREF(...) \
do { \
  if (solver && GET_OPTION (log)) \
    kissat_log_ref (solver, __VA_ARGS__); \
} while (0)

#define LOGBINARY(...) \
do { \
  if (solver && GET_OPTION (log)) \
    kissat_log_binary (solver, __VA_ARGS__); \
} while (0)

#define LOGBINARY3(...) \
do { \
  if (GET_OPTION (log) > 2) \
    kissat_log_binary (solver, __VA_ARGS__); \
} while (0)

#define LOGUNARY(...) \
do { \
  if (solver && GET_OPTION (log)) \
    kissat_log_unary (solver, __VA_ARGS__); \
} while (0)

#define LOGLIT(LIT) kissat_log_lit (solver, (LIT))

#define LOGWATCH(...) \
do { \
  if (GET_OPTION (log)) \
    kissat_log_watch (solver, __VA_ARGS__); \
} while (0)

#define LOGXOR(...) \
do { \
  if (GET_OPTION (log)) \
    kissat_log_xor (solver, __VA_ARGS__); \
} while (0)

#else

#define LOG(...) do { } while (0)
#define LOG2(...) do { } while (0)
#define LOG3(...) do { } while (0)
#define LOG4(...) do { } while (0)
#define LOG5(...) do { } while (0)
#define LOGRES(...) do { } while (0)
#define LOGRES2(...) do { } while (0)
#define LOGLITS(...) do { } while (0)
#define LOGLITS3(...) do { } while (0)
#define LOGEXT(...) do { } while (0)
#define LOGEXT2(...) do { } while (0)
#define LOGINTS(...) do { } while (0)
#define LOGINTS3(...) do { } while (0)
#define LOGUNSIGNEDS3(...) do { } while (0)
#define LOGCLS(...) do { } while (0)
#define LOGCLS2(...) do { } while (0)
#define LOGCLS3(...) do { } while (0)
#define LOGREF(...) do { } while (0)
#define LOGBINARY(...) do { } while (0)
#define LOGBINARY3(...) do { } while (0)
#define LOGUNARY(...) do { } while (0)
#define LOGWATCH(...) do { } while (0)
#define LOGXOR(...) do { } while (0)

#endif

#define LOGTMP(...) \
  LOGLITS (SIZE_STACK (solver->clause.lits),  \
           BEGIN_STACK (solver->clause.lits), __VA_ARGS__)

#endif
