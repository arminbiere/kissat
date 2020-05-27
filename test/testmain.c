#include "test.h"

#include "../src/handle.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void
test_main_version (void)
{
  const size_t len = strlen (tissat_root) + 32;
  char *cmd = malloc (len);
  sprintf (cmd, "%s/kissat --banner", tissat_root);
  tissat_verbose ("executing 'system (\"%s\")':", cmd);
  tissat_verbose ("");
  int wstatus = system (cmd);
  tissat_verbose ("");
  if (WIFSIGNALED (wstatus))
    {
      const int sig = WTERMSIG (wstatus);
      const char *name = kissat_signal_name (sig);
      FATAL ("caught unexpected signal '%d' (%s) in 'system (\"%s\")'",
	     sig, name, cmd);
    }
  else if (!WIFEXITED (wstatus))
    FATAL ("could not get exit status of 'system (\"%s\")'", cmd);
  else
    {
      int status = WEXITSTATUS (wstatus);
      if (status)
	FATAL ("unexpected exit status '%d' of 'system (\"%s\")'",
	       status, cmd);
      else
	tissat_verbose ("exit status '0' as expected");
    }
  free (cmd);
}

#ifdef _POSIX_C_SOURCE

static void
execute_solver_and_send_signal (int sig)
{
  const size_t len = strlen (tissat_root) + 32;
  char *path = malloc (len);
  sprintf (path, "%s/kissat", tissat_root);
  const char *arg = "../test/cnf/hard.cnf";
  tissat_verbose ("executing '%s %s'", path, arg);

  const char *name = kissat_signal_name (sig);
  tissat_verbose ("sending and catching signal %d ('%s')", sig, name);

  int child = fork ();

  if (child < 0)
    FATAL ("failed to fork child process");
  else if (child)
    {
      const unsigned micro_seconds = 1e4 * (5 + (((unsigned) sig) % 10));
      tissat_verbose ("sleeping %u micro seconds "
		      "before sending signal '%d' (%s)",
		      micro_seconds, sig, name);
      if (usleep (micro_seconds))
	tissat_warning ("could not execute 'usleep' "
			"before sending signal '%d' (%s)", sig, name);
      else
	{
	  if (kill (child, sig))
	    tissat_warning ("failed to send signal '%d' (%s)", sig, name);
	  else
	    {
	      int wstatus;
	      pid_t pid = waitpid (child, &wstatus, 0);
	      if (pid != child)
		FATAL ("failed to wait on child process");
	      if (sig == SIGALRM)
		{
		  if (WIFSIGNALED (wstatus))
		    {
		      const int term_sig = WTERMSIG (wstatus);
		      FATAL ("child terminated by signal '%d' (%s) "
			     "but expected it to exit",
			     term_sig, kissat_signal_name (term_sig));
		    }
		  else if (!WIFEXITED (wstatus))
		    FATAL ("could not get exit status of child");
		  else
		    {
		      int status = WEXITSTATUS (wstatus);
		      if (status)
			FATAL ("child exit status '%d' but expected '0'",
			       status);
		      else
			tissat_verbose ("child exit status '0' as expected");
		    }
		}
	      else
		{
		  if (WIFEXITED (wstatus))
		    FATAL ("child exited with '%d' "
			   "but expected signal '%d' (%s)",
			   WEXITSTATUS (wstatus), sig, name);
		  else if (!WIFSIGNALED (wstatus))
		    FATAL
		      ("child not signalled but expected signal '%d' (%s)",
		       sig, name);
		  else
		    {
		      int term_sig = WTERMSIG (wstatus);
		      if (term_sig != sig)
			FATAL ("child terminated by signal '%d' (%s) "
			       "and not as expected by '%d' (%s)",
			       term_sig, kissat_signal_name (term_sig),
			       sig, name);
		      else
			tissat_verbose ("caught signal '%d' (%s) as expected",
					sig, name);
		    }
		}
	    }
	}
    }
  else
    {
      kissat_reset_signal_handler ();
      execl (path, path, arg, (char *) 0);
      exit (0);
    }

  free (path);
}

#ifdef ASAN

#define SIGNALS \
SIGNAL(SIGABRT) \
SIGNAL(SIGALRM) \
SIGNAL(SIGINT) \
SIGNAL(SIGTERM)

#else

#define SIGNALS \
SIGNAL(SIGABRT) \
SIGNAL(SIGALRM) \
SIGNAL(SIGINT) \
SIGNAL(SIGSEGV) \
SIGNAL(SIGTERM)

#endif

#define SIGNAL(NAME) \
\
static void \
test_main_ ## NAME (void) \
{ \
  execute_solver_and_send_signal (NAME); \
}

SIGNALS
#undef SIGNAL
#endif
  void
tissat_schedule_main (void)
{
  SCHEDULE_FUNCTION (test_main_version);
#ifdef _POSIX_C_SOURCE
  if (tissat_found_test_directory)
    {
#define SIGNAL(NAME) SCHEDULE_FUNCTION (test_main_ ## NAME);
      SIGNALS
#undef SIGNAL
    }
#endif
}
