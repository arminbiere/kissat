#include "../src/file.h"

#include "test.h"

void tissat_schedule_usage (void) {
#define APP tissat_schedule_application

  APP (0, "-h");
  APP (0, "--help");
  APP (0, "--banner");
  APP (0, "--id");
  APP (0, "--compiler");
  APP (0, "--version");

#ifndef NOPTIONS

#ifdef EMBEDDED
  APP (0, "--embedded");
#endif
  APP (0, "--range");

  if (tissat_found_test_directory) {
    APP (20, "--color ../test/cnf/add8.cnf");
    APP (20, "--colors ../test/cnf/add8.cnf");
    APP (20, "--colors ../test/cnf/add8.cnf");
    APP (20, "--colours ../test/cnf/add8.cnf");

    APP (20, "--no-color ../test/cnf/add8.cnf");
    APP (20, "--no-colors ../test/cnf/add8.cnf");
    APP (20, "--no-colors ../test/cnf/add8.cnf");
    APP (20, "--no-colours ../test/cnf/add8.cnf");

    APP (20, "../test/cnf/add8.cnf --no-simplify");

    if (kissat_file_readable ("../test/cnf/add32.cnf"))
      APP (20, "../test/cnf/add32.cnf --no-compact");

    APP (20, "../test/cnf/add8.cnf --no-probe");
    APP (20, "../test/cnf/add8.cnf --no-substitute");

    APP (20, "../test/cnf/add8.cnf --no-eliminate");

    APP (20, "../test/cnf/add8.cnf --stable=2");
    APP (20, "../test/cnf/add8.cnf --no-stable");

    APP (20, "../test/cnf/add8.cnf --probeinit=0 --no-vivify");

    APP (20, "../test/cnf/add8.cnf --eliminateinit=0 --no-extract");
    APP (20, "../test/cnf/add8.cnf --eliminateinit=0 --no-ifthenelse");
    APP (20, "../test/cnf/add8.cnf --eliminateinit=0 --no-equivalences");
    APP (20, "../test/cnf/add8.cnf --eliminateinit=0 --no-ands");

#ifndef QUIET
    APP (0, "--walkinitially --conflicts=3000 --probeinit=0 "
            "--eliminateinit=0 ../test/cnf/hard.cnf --profile=4");
    APP (0, "../test/cnf/hard.cnf --walkinitially -v -v -v "
            "--colors --conflicts=1e4");
#endif

    APP (0, "--decisions=10 ../test/cnf/hard.cnf --no-reduce");
    APP (0, "--decisions=10 ../test/cnf/hard.cnf --no-rephase");
    APP (0, "--decisions=10 ../test/cnf/hard.cnf --no-restart");
    APP (0, "--decisions=10 ../test/cnf/hard.cnf "
            "--reluctantint=200 --reluctantlim=100 --stable=2");
    APP (0, "--decisions=1000 ../test/cnf/hard.cnf "
            "--no-reluctant --stable=2");
  }

#else

#ifdef SAT
  if (tissat_found_test_directory)
    APP (20, "../test/cnf/add8.cnf --sat");
#elif defined(UNSAT)
  if (tissat_found_test_directory)
    APP (20, "../test/cnf/add8.cnf --unsat");
#else
  if (tissat_found_test_directory)
    APP (20, "../test/cnf/add8.cnf --default");
#endif

#endif

#ifdef NOPTIONS
#define LIMITED_OPTIONS ""
#else
#define LIMITED_OPTIONS " --rephaseinit=10 --rephaseint=10"
#endif

  if (tissat_found_test_directory) {
    APP (0, "--conflicts=6e3 ../test/cnf/hard.cnf" LIMITED_OPTIONS);
    APP (0, "--decisions=8e3 ../test/cnf/hard.cnf" LIMITED_OPTIONS);
    APP (0, "--conflicts=7e3 --decisions=7e3 "
            "../test/cnf/hard.cnf" LIMITED_OPTIONS);
  }

  APP (1, "--help -n");
  APP (1, "--version -n");
  APP (1, "-n --version");
  APP (1, "-n -h");

#ifdef QUIET
  APP (1, "-q");
  APP (1, "-s");
  APP (1, "-v");

#endif

#ifndef LOGGING
  APP (1, "-l");
#endif

#if !defined(QUIET) && defined(NOPTIONS)
  APP (1, "-l");
  APP (1, "-q");
  APP (1, "-s");
  APP (1, "-v");
#endif

#ifdef NOPTIONS
  APP (1, "--statistics");
#endif

  APP (1, "--invalid");
  APP (1, "-X");

  APP (1, "three command-line arguments");
  APP (1, "/dev/null /dev/null /dev/null");

#undef APP
}
