#include "test.h"

#include "../src/application.h"
#include "../src/file.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static char *
copy_string (const char *begin, const char *end)
{
  const size_t len = end - begin;
  char *res = malloc (len + 1);
  memcpy (res, begin, len);
  res[len] = 0;
  return res;
}

void
tissat_call_application (int expected, const char *cmd)
{
#define MAX_ARGC 8
  char *argv[MAX_ARGC];
  int argc = 0;
  argv[argc++] = "kissat";
  for (const char *p = cmd, *start = cmd;; p++)
    if (!*p || *p == ' ')
      {
	if (argc == MAX_ARGC)
	  FATAL ("MAX_ARGC exceeded");
	argv[argc++] = copy_string (start, p);
	if (!*p)
	  break;
	start = ++p;
      }
#undef MAX_ARGC
  kissat *solver = kissat_init ();
  tissat_init_solver (solver);
  tissat_redirect_stderr_to_stdout ();
  int res = kissat_application (solver, argc, argv);
  tissat_restore_stderr ();
  if (res != expected)
    FATAL ("'kissat %s' returns '%d' and not '%d'", cmd, res, expected);
  kissat_release (solver);
  for (int i = 1; i < argc; i++)
    free (argv[i]);
  tissat_verbose ("Application 'kissat %s' returned '%d' as expected.",
		  cmd, res);
}

const char *tissat_options[] = {
  "",
#if !defined(QUIET) && !defined(NOPTIONS)
  "-q ",
  "-s ",
  "-v ",
  "-s -v ",
#endif
};

#define SIZE_OPTIONS (sizeof (tissat_options) / sizeof (char*))
const unsigned tissat_size_options = SIZE_OPTIONS;
const char **tissat_end_of_options = tissat_options + SIZE_OPTIONS;

const char *
tissat_next_option (unsigned count)
{
  assert (tissat_size_options);
  return tissat_options[count % tissat_size_options];
}
