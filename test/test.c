#define DEFAULT_NUMBER_OF_PROCESSES 4

static const char *usage =
  "usage: tissat [<option> ... ] [ <pattern> ... ]\n"
  "\n"
  "where '<option>' is one of the following:\n"
  "\n"
  "-h               prints this command line option information\n"
  "-v               increase verbosity and force sequential testing\n"
#if defined(LOGGING) && !defined(QUIET)
  "-l               increase logging level for solver instances\n"
  "-s               non-parallel sequential testing"
  "(implied by '-v' and '-l')\n"
  "-p               print progress "
  "(can not be combined with '-v' or '-l')\n"
#else
  "-p               print progress (can not be combined with '-v')\n"
  "-s               non-parallel sequential testing (implied by '-v')\n"
#endif
  "-j[<processes>]  number of processes (infinite '-j', default '-j%d')\n"
  "-b               thorough big tests and all options\n"
  "\n"
  "The list of patterns is matched to the function names of the tests.\n"
  "If no pattern is given at all then all test cases are executed.\n"
  "\n"
  "Otherwise only those tests are executed for which at least one\n"
  "pattern matches its function name (contains the pattern).\n";

#include "build.h"

#include "../src/allocate.h"
#include "../src/application.h"
#include "../src/check.h"
#include "../src/clause.h"
#include "../src/colors.h"
#include "../src/file.h"
#include "../src/handle.h"
#include "../src/internal.h"
#include "../src/literal.h"
#include "../src/parse.h"
#include "../src/print.h"
#include "../src/resize.h"
#include "../src/resources.h"
#include "../src/stack.h"
#include "../src/utilities.h"
#include "../src/vector.h"

#include <ctype.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

#include "test.h"

bool tissat_big;
bool tissat_found_test_directory;
bool tissat_sequential;
bool tissat_progress;

int tissat_processes;

#ifndef NPROOFS

bool tissat_found_drabt;
bool tissat_found_drat_trim;

#endif

const char *tissat_root = DIR;

#ifdef _POSIX_C_SOURCE

bool tissat_found_bzip2;
bool tissat_found_gzip;
bool tissat_found_lzma;
bool tissat_found_xz;
bool tissat_found_7z;

#endif

#if defined(LOGGING) && !defined(QUIET)
static int tissat_logging;
#endif

void
tissat_init_solver (kissat * solver)
{
#if !defined(NOPTIONS) && !defined(QUIET)
#ifdef LOGGING
  if (tissat_logging)
    solver->options.log = tissat_logging;
#endif
  if (tissat_verbosity > 1)
    solver->options.verbose = tissat_verbosity - 1;
#else
  (void) solver;
#endif
  solver->watching = true;
}

static bool
find_test_directory (void)
{
  struct stat buf;
  return !stat ("../test", &buf);
}

static bool
match (const char *str, const char *pattern)
{
  for (const char *s = str; *s; s++)
    {
      const char *p = pattern;
      for (const char *q = s; *p && *p == *q; p++, q++)
	;
      if (!*p)
	return true;
    }
  return false;
}

static int patterns;

static void
schedule (const char *name, void (*scheduler) (void), int argc, char **argv)
{
  bool execute = !patterns || argc < 2;
  for (int i = 1; !execute && i < argc; i++)
    execute = match (name, argv[i]);
  if (patterns && !execute)
    return;
  const unsigned before = tissat_scheduled;
  scheduler ();
  const unsigned jobs = tissat_scheduled - before;
  tissat_message ("Scheduled %u jobs through '%s'.", jobs, name);
}

static int
get_number_of_cores (void)
{
  int res = -1;
#ifdef _POSIX_C_SOURCE
  res = sysconf (_SC_NPROCESSORS_ONLN);
#endif
  return res;
}

static int
default_number_of_processes (void)
{
  int res = get_number_of_cores ();
  return res <= 0 ? DEFAULT_NUMBER_OF_PROCESSES : res;
}

int
main (int argc, char **argv)
{
  const double start = kissat_wall_clock_time ();
  const char *sequential_option = 0;
  const char *processes_option = 0;
  const char *verbose_option = 0;

  if (chdir (DIR))
    FATAL ("could not change to build director '%s'", DIR);

  for (int i = 1; i < argc; i++)
    if (!strcmp (argv[i], "-h"))
      printf (usage, default_number_of_processes ()), exit (0);
    else if (!strcmp (argv[i], "-v"))
      {
	if (tissat_progress)
	  tissat_error ("can not combine '-p' and '-v'");

	if (processes_option)
	  tissat_error ("can not combine '%s' and '-v'", processes_option);

	if (!verbose_option)
	  verbose_option = argv[i];

	tissat_verbosity++;
      }
    else if (!strcmp (argv[i], "-s"))
      {
	if (sequential_option)
	  tissat_error ("multiple '-s' options");

	if (processes_option)
	  tissat_error ("can not combine '%s' and '-s'", processes_option);

	if (!tissat_sequential)
	  {
	    tissat_sequential = true;
	    sequential_option = "-s";
	  }
      }
    else if (!strcmp (argv[i], "-p"))
      {
	if (verbose_option)
	  tissat_error ("can not combine '%s' and '-p'", verbose_option);

	tissat_progress = true;
      }
    else if (!strcmp (argv[i], "-j"))
      {
	if (verbose_option)
	  tissat_error ("can not combine '%s' and '-j'", verbose_option);

	if (tissat_sequential)
	  tissat_error ("can not combine '-s' and '-j'");

	if (processes_option)
	  {
	    if (!strcmp (processes_option, "-j"))
	      tissat_error ("multiple '-j' options");
	    else
	      tissat_error ("multiple '%s' and '-j'", processes_option);
	  }
	else
	  assert (!tissat_processes);

	processes_option = argv[i];
	tissat_processes = -1;
      }
    else if (argv[i][0] == '-' && argv[i][1] == 'j' && argv[i][2])
      {
	if (processes_option)
	  {
	    if (!strcmp (processes_option, argv[i]))
	      tissat_error ("multiple '%s' options", argv[i]);
	    else
	      tissat_error ("multiple '%s' and '%s'",
			    processes_option, argv[i]);
	  }
	else
	  assert (!tissat_processes);

	int tmp = 0;
	char ch;
	for (const char *p = argv[i] + 2; (ch = *p); p++)
	  {
	    if (!isdigit (ch))
	      tissat_error ("expected digit in '%s' (try '-h')", argv[i]);
	    if (INT_MAX / 10 < tmp)
	      tissat_error ("number in '%s' way too large (try '-h')",
			    argv[i]);
	    tmp *= 10;
	    int digit = ch - '0';
	    if (INT_MAX - digit <= tmp)
	      tissat_error ("number in '%s' too large (try '-h')", argv[i]);
	    tmp += digit;
	  }
	if (!tmp)
	  tissat_error ("invalid zero argument in '%s' (try '-h')", argv[i]);

	if (verbose_option)
	  tissat_error ("can not combine '%s' and '%s'",
			verbose_option, argv[i]);

	if (tissat_sequential)
	  tissat_error ("can not combine '-s' and '%s'", argv[i]);

	processes_option = argv[i];
	tissat_processes = tmp;
      }
#if defined(LOGGING) && !defined(QUIET)
    else if (!strcmp (argv[i], "-l"))
      {
	if (tissat_progress)
	  tissat_error ("can not combine '-p' and '-l");
	if (processes_option)
	  tissat_error ("can not combine '%s' and '-v'", processes_option);
	if (!verbose_option)
	  verbose_option = argv[i];
	tissat_logging++;
	tissat_verbosity = 4;
      }
#endif
    else if (!strcmp (argv[i], "-b"))
      tissat_big = true;
    else if (argv[i][0] == '-')
      tissat_error ("invalid option '%s' (try '-h')", argv[i]);
    else
      patterns++;

  if (tissat_verbosity && !tissat_sequential)
    {
      if (!tissat_sequential)
	{
	  tissat_sequential = true;
	  sequential_option = "-v";
	}
    }

#if defined(LOGGING)
  if (tissat_logging && !tissat_sequential)
    {
      if (!tissat_sequential)
	{
	  tissat_sequential = true;
	  sequential_option = "-l";
	}
    }
#endif

  if (!tissat_sequential && !tissat_processes)
    tissat_processes = default_number_of_processes ();

#ifndef QUIET
  kissat_banner ("", "TISSAT Tester for KISSAT");
  tissat_line ();
  tissat_message ("Use '-h' to print usage (i.e., how to use patterns).");
#else
  tissat_message ("TISSAT Tester for KISSAT");
#endif
  tissat_message ("Changed to '%s' directory.", DIR);

  if (tissat_sequential)
    tissat_message ("Forced sequential non-parallel execution "
		    "(due to '-s').");
  else if (tissat_processes < 0)
    {
      if (processes_option)
	tissat_message ("Parallel execution "
			"using an arbitrary number of processes "
			"(due to '-j').");
      else
	{
	  assert (DEFAULT_NUMBER_OF_PROCESSES < 0);
	  tissat_message ("Parallel execution "
			  "using an arbitrary number of processes "
			  "(by default).");
	}
    }
  else
    {
      if (processes_option)
	tissat_message ("Parallel execution "
			"using at most %d processes (due to '%s').",
			tissat_processes, processes_option);
      else
	{
	  tissat_message ("Parallel execution "
			  "using at most %d processes (by default).",
			  tissat_processes);
	}
    }

  if (tissat_progress)
    tissat_message ("Job execution progress reporting " "(due to '-p').");
  else if (verbose_option)
    tissat_message ("Job execution progress reporting disabled "
		    "(due to '%s').", verbose_option);
  else
    tissat_message ("Job execution progress reporting disabled "
		    "(enable with '-p').");

  tissat_found_test_directory = find_test_directory ();

  if (!tissat_found_test_directory)
    {
      tissat_line ();
      tissat_message ("Could not find '../test/' directory.");
      tissat_message ("Skipping test cases that rely on '../test' .");
    }
  else
    {
      tissat_line ();
      tissat_message ("Found '../test' directory "
		      "(running test cases that need '../test' too).");
    }

#ifndef NPROOFS
  bool found_proof_checker = false;
#define FIND(EXECUTABLE,FLAG) \
do { \
  if ((tissat_found_ ## FLAG = kissat_find_executable (#EXECUTABLE))) \
    { \
      tissat_message ("Found '%s' executable (will check proofs with it).", \
	      #EXECUTABLE); \
      found_proof_checker = true; \
    } \
  else \
    tissat_warning ("Did not find '%s' executable.", #EXECUTABLE); \
} while (0)

  // *INDENT-OFF*

  FIND (drabt, drabt);
  FIND (drat-trim, drat_trim);

  // *INDENT-ON*

  if (!found_proof_checker)
    {
      tissat_line ();
      tissat_bold_message ("Install either 'drat-trim' or "
			   "'drabt' to check proofs too!");
      tissat_line ();
    }
#undef FIND
#endif

#ifdef _POSIX_C_SOURCE
  bool found_compression_utility;
#define FIND(EXECUTABLE) \
do { \
  if ((tissat_found_ ## EXECUTABLE = kissat_find_executable (#EXECUTABLE))) \
    { \
      tissat_message ("Found '%s' executable for testing compression.", \
              #EXECUTABLE); \
      found_compression_utility = true; \
    } \
  else \
    tissat_warning ("Did not find '%s' executable for testing compression.", \
            #EXECUTABLE); \
} while (0)

  // *INDENT-OFF*

  FIND (bzip2);
  FIND (gzip);
  FIND (lzma);
  FIND (xz);
  FIND (7z);

  // *INDENT-ON*

  if (!found_compression_utility)
    tissat_message
      ("Not testing compression (no compression utility found).");

#else
  tissat_message ("No compression tests for non POSIX configuration.");
#endif

  tissat_line ();

#define SCHEDULE(NAME) \
do { \
  void tissat_schedule_ ## NAME (void); \
  schedule ("tissat_schedule_" #NAME, \
            tissat_schedule_ ## NAME, argc, argv); \
} while (0)

  SCHEDULE (error);
  SCHEDULE (endianess);
  SCHEDULE (format);
  SCHEDULE (references);
  SCHEDULE (reluctant);
  SCHEDULE (random);
  SCHEDULE (queue);
  SCHEDULE (allocate);
  SCHEDULE (stack);
  SCHEDULE (arena);
  SCHEDULE (heap);
  SCHEDULE (vector);
  SCHEDULE (rank);
  SCHEDULE (sort);
  SCHEDULE (bump);
  SCHEDULE (options);
  SCHEDULE (init);
  SCHEDULE (add);
  SCHEDULE (file);
  SCHEDULE (parse);
  SCHEDULE (usage);
  SCHEDULE (main);
  SCHEDULE (collect);
  SCHEDULE (solve);
  SCHEDULE (coverage);
  SCHEDULE (terminate);

#ifndef NPROOFS
  if (tissat_found_drabt || tissat_found_drat_trim)
    SCHEDULE (prove);
#endif

#ifndef NDEBUG
  SCHEDULE (dump);
#endif

  if (tissat_scheduled)
    {
      tissat_line ();

      if (tissat_sequential)
	tissat_run_jobs (0);
      else
	tissat_run_jobs (tissat_processes);

      if (tissat_verbosity)
	tissat_section ("Finished");
      else
	tissat_line ();
      tissat_message ("All %u test jobs %ssucceeded%s in %.2f seconds.",
		      tissat_scheduled,
		      kissat_bold_green_color_code (1),
		      kissat_normal_color_code (1),
		      kissat_wall_clock_time () - start);
      tissat_release_jobs ();
    }
  else
    tissat_warning ("No job scheduled.");

  return 0;
}
