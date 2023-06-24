#ifndef _testscheduler_h_INCLUDED
#define _testscheduler_h_INCLUDED

typedef struct tissat_job tissat_job;

extern unsigned tissat_scheduled;

void tissat_schedule_function (void (*function) (void), const char *name);
tissat_job *tissat_schedule_command (int, const char *command,
                                     tissat_job *);
tissat_job *tissat_schedule_application (int, const char *args);

#define SCHEDULE_FUNCTION(FUNCTION) \
  tissat_schedule_function (FUNCTION, #FUNCTION)

void tissat_run_jobs (int parallel);
void tissat_release_jobs (void);

#endif
