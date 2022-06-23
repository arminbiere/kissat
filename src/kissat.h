#ifndef _kissat_h_INCLUDED
#define _kissat_h_INCLUDED

#include <unistd.h>

typedef struct kissat kissat;

// Default (partial) IPASIR interface.

DllExport const char *kissat_signature (void);
DllExport kissat *kissat_init (void);
DllExport void kissat_add (kissat * solver, int lit);
DllExport int kissat_solve (kissat * solver);
DllExport void kissat_terminate (kissat * solver);
DllExport int kissat_value (kissat * solver, int lit);
DllExport void kissat_release (kissat * solver);

DllExport void kissat_set_terminate (kissat * solver,
			   void *state, int (*terminate) (void *state));

// Additional API functions.

DllExport void kissat_reserve (kissat * solver, int max_var);

DllExport const char *kissat_id (void);
DllExport const char *kissat_version (void);
DllExport const char *kissat_compiler (void);

DllExport const char **kissat_copyright (void);
DllExport void kissat_build (const char *line_prefix);
DllExport void kissat_banner (const char *line_prefix, const char *name_of_app);

DllExport int kissat_get_option (kissat * solver, const char *name);
DllExport int kissat_set_option (kissat * solver, const char *name, int new_value);

DllExport int kissat_has_configuration (const char *name);
DllExport int kissat_set_configuration (kissat * solver, const char *name);

DllExport void kissat_set_conflict_limit (kissat * solver, unsigned);
DllExport void kissat_set_decision_limit (kissat * solver, unsigned);

DllExport void kissat_print_statistics (kissat * solver);

#endif
