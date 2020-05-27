#ifndef _tissatapplication_h_INCLUDED
#define _tissatapplication_h_INCLUDED

struct tissat_job;

extern const char *tissat_options[];
extern const char **tissat_end_of_options;

void tissat_call_application (int expected, const char *cmd);

const char *tissat_next_option (unsigned count);

#define all_tissat_options(OPT) \
  const char * OPT, ** PTR_ ## OPT = tissat_options; \
  PTR_ ## OPT != tissat_end_of_options && (OPT = *PTR_ ##OPT, true); \
  PTR_ ## OPT++

#endif
