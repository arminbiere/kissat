#include "test.h"

#include "../src/error.h"
#include "../src/handle.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void
test_real_fatal_error (void)
{
  int child = fork ();
  if (child < 0)
    FATAL ("failed to fork child process");
  else if (child)
    {
      int wstatus;
      pid_t pid = waitpid (child, &wstatus, 0);
      if (pid != child)
	FATAL ("failed to wait on child process");
      else if (WIFEXITED (wstatus))
	FATAL ("child exited");
      else if (!WIFSIGNALED (wstatus))
	FATAL ("child not signalled");
      else
	{
	  int sig = WTERMSIG (wstatus);
	  if (sig != SIGABRT)
	    FATAL ("child terminated by signal '%d' (%s) "
		   "and not as expected by '%d' (SIGABRT)",
		   sig, kissat_signal_name (sig), (int) (SIGABRT));
	}
    }
  else
    {
      kissat_reset_signal_handler ();
      kissat_fatal ("real fatal error message triggering 'abort'");
      exit (0);
    }
}

#include <setjmp.h>

static jmp_buf jump_buffer;

static void
abort_call_back (void)
{
  longjmp (jump_buffer, 42);
}

static void
test_fake_fatal_error (void)
{
  kissat_call_function_instead_of_abort (abort_call_back);
  int val = setjmp (jump_buffer);
  if (val)
    {
      kissat_call_function_instead_of_abort (0);
      if (val != 42)
	FATAL ("expected '42' as result from 'setjmp'");
    }
  else
    {
      kissat_fatal ("faked fatal error message triggering long jump");
      kissat_call_function_instead_of_abort (0);
      FATAL ("long jump not taken");
    }
}

static void
just_return_call_back (void)
{
}

static void
test_just_return_from_fatal_error (void)
{
  kissat_call_function_instead_of_abort (just_return_call_back);
  kissat_fatal ("faked just returning fatal error message");
}

static void
test_start_of_fatal_error_message (void)
{
  kissat_fatal_message_start ();
  printf ("after starting a fatal error message printing this message\n");
  fflush (stdout);
}

void
tissat_schedule_error (void)
{
  SCHEDULE_FUNCTION (test_real_fatal_error);
  SCHEDULE_FUNCTION (test_fake_fatal_error);
  SCHEDULE_FUNCTION (test_just_return_from_fatal_error);
  SCHEDULE_FUNCTION (test_start_of_fatal_error_message);
}
