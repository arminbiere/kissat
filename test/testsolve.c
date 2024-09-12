#include "../src/file.h"

#include "test.h"
#include "testcnfs.h"

static unsigned scheduled;

static void schedule_solve_job_with_option (int expected, const char *opt,
                                            const char *path) {
  if (!kissat_file_readable (path)) {
    tissat_warning ("Skipping unreadable '%s'", path);
    return;
  }
  size_t len = strlen (opt) + strlen (path) + 1;
  char * cmd = malloc (len);
  strcpy (cmd, opt);
  strcat (cmd, path);
  tissat_schedule_application (expected, cmd);
  free (cmd);
  scheduled++;
}

static const char *simps[] = {
    "",
#ifndef NOPTIONS
    "--eliminateinit=0 ",
    "--probeinit=0 ",
    "--reduceinit=10 --rephaseinit=10 --rephaseint=10 ",
    "--incremental ",
    "--walkinitially ",
#endif
};

static const char **end_of_simps = simps + sizeof (simps) / sizeof *simps;

static void schedule_solve_job (int expected, const char *path) {
  for (const char **p = simps; p != end_of_simps; p++) {
    char combined[128];
    if (tissat_big) {
      for (all_tissat_options (opt)) {
        sprintf (combined, *p, opt);
        schedule_solve_job_with_option (expected, combined, path);
      }
    } else {
      const char *opt = tissat_next_option (scheduled);
      sprintf (combined, *p, opt);
      schedule_solve_job_with_option (expected, combined, path);
    }
  }
}

void tissat_schedule_solve (void) {
#define CNF(EXPECTED, NAME, BIG) \
  if (!BIG || tissat_big) \
    schedule_solve_job (EXPECTED, "../test/cnf/" #NAME ".cnf");
  CNFS
#undef CNF
}
