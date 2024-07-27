#include "../src/handle.h"
#include "../src/print.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "test.h"

#define MAX_JOBS (1u << 13)

struct tissat_job {
  pid_t pid;
  unsigned id;
  int expected;
  bool executed;
  bool finished;
  char *command;
  char *application;
  void (*function) (void);
  tissat_job *dependency;
  const char *name;
};

static tissat_job jobs[MAX_JOBS];

unsigned tissat_scheduled;

static unsigned executed;
static unsigned finished;

static tissat_job *new_job (int expected) {
  if (tissat_scheduled == MAX_JOBS)
    tissat_fatal ("maximum number %u of scheduled jobs exhausted",
                  MAX_JOBS);
  tissat_job *res = jobs + tissat_scheduled;
  res->id = tissat_scheduled++;
  res->expected = expected;
  return res;
}

void tissat_schedule_function (void (*function) (void), const char *name) {
  tissat_job *res = new_job (0);
  res->function = function;
  res->name = name;
}

tissat_job *tissat_schedule_application (int expected, const char *args) {
  tissat_job *res = new_job (expected);
  strcpy (res->application = malloc (strlen (args) + 1), args);
  return res;
}

tissat_job *tissat_schedule_command (int expected, const char *command,
                                     tissat_job *dependency) {
  tissat_job *res = new_job (expected);
  strcpy (res->command = malloc (strlen (command) + 1), command);
  res->dependency = dependency;
  return res;
}

static void execute_function (tissat_job *job) {
  tissat_section ("Executing Function '%s'", job->name);
  job->function ();
}

static void execute_application (tissat_job *job) {
  tissat_section ("Executing Application 'kissat %s'", job->application);
  tissat_call_application (job->expected, job->application);
}

static int check_command (tissat_job *job, int status) {
  assert (job);
  assert (job->command);
  int res = 0;
  if (WIFEXITED (status)) {
    res = WEXITSTATUS (status);
    if (res == job->expected)
      tissat_verbose ("Command '%s' returned '%d' as expected.",
                      job->command, res);
    else
      tissat_error ("Command '%s' returns '%d' and not '%d'", job->command,
                    res, job->expected);
  } else if (WIFSIGNALED (status))
    tissat_signal (WTERMSIG (status), "executing command '%s",
                   job->command);
  else
    tissat_error ("Unexpected return status of command '%s'");
  return res;
}

static void check_function (tissat_job *job, int status) {
  assert (job);
  assert (job->function);
  if (status)
    tissat_error ("Function job '%s' failed with exit status '%d",
                  job->name, status);
}

static void check_application (tissat_job *job, int status) {
  assert (job);
  assert (job->application);
  if (status)
    tissat_error ("Application job 'kissat %s' failed with exit status '%d",
                  job->application, status);
}

static int execute_command (tissat_job *job) {
  tissat_section ("Executing Command '%s'", job->command);
  int status = system (job->command);
  int res = 0;
  if (status < 0)
    tissat_error ("Could not generate child process or retrieve status "
                  "while trying to execute command '%s'",
                  job->command);
  else if (status == 127)
    tissat_error ("Shell could not be executed in the child process "
                  "while trying to execute command '%s'",
                  job->command);
  else
    res = check_command (job, status);
  return res;
}

static tissat_job *running_job;

static int execute_job (tissat_job *job) {
  int res = 0;
  running_job = job;
  if (job->function)
    execute_function (job);
  else if (job->application)
    execute_application (job);
  else {
    assert (job->command);
    res = execute_command (job);
  }
  return res;
}

static void handle_signal (tissat_job *job, int sig) {
  if (!job)
    tissat_signal (sig, "but could not find corresponding job");
  else if (job->function)
    tissat_signal (sig, "in function '%s'", job->name);
  else if (job->command)
    tissat_signal (sig, "in command '%s'", job->command);
  else {
    assert (job->application);
    tissat_signal (sig, "in application 'kissat %s'", job->application);
  }
}

static void handle_exit (tissat_job *job, int status) {
  if (!job)
    tissat_fatal ("exit status '%d' "
                  "but could not find corresponding job",
                  status);
  if (job->function)
    check_function (job, status);
  else if (job->application)
    check_application (job, status);
  else
    check_command (job, status);
}

static void sequential_signal_handler (int sig) {
  kissat_reset_signal_handler ();
  tissat_restore_stdout_and_stderr ();
  if (!running_job)
    tissat_signal (sig, "but no job seems to run");
  handle_signal (running_job, sig);
}

static void set_sequential_signal_handler (void) {
  kissat_init_signal_handler (sequential_signal_handler);
}

static void reset_signal_handler (void) { kissat_reset_signal_handler (); }

static void sequential_progress (void) {
  if (!tissat_progress)
    return;
  printf ("sequential: executed %u, finished %u\n", executed, finished);
  fflush (stdout);
}

static void run_sequential_job (tissat_job *job) {
  executed++;
  sequential_progress ();
  tissat_divert_stdout_and_stderr_to_dev_null ();
  set_sequential_signal_handler ();
  execute_job (job);
  finished++;
  reset_signal_handler ();
  tissat_restore_stdout_and_stderr ();
  sequential_progress ();
}

static void run_parallel_job (tissat_job *job) {
  tissat_divert_stdout_and_stderr_to_dev_null ();
  int res = execute_job (job);
  tissat_restore_stdout_and_stderr ();
  exit (res);
}

#define all_jobs(JOB) \
  tissat_job *JOB = jobs, *END_##JOB = JOB + tissat_scheduled; \
  JOB != END_##JOB; \
  JOB++

static void run_jobs_sequentially (void) {
  tissat_message ("Running %u jobs sequentially all in the same process.",
                  tissat_scheduled);
  for (all_jobs (job))
    run_sequential_job (job);
}

static unsigned search_executed;

static tissat_job *find_executable_job (void) {
  while (assert (search_executed < tissat_scheduled),
         jobs[search_executed].executed)
    search_executed++;
  for (unsigned i = search_executed; i < tissat_scheduled; i++) {
    tissat_job *job = jobs + i;
    if (job->executed)
      continue;
    tissat_job *dependency = job->dependency;
    if (!dependency)
      return job;
    if (dependency->finished)
      return job;
  }
  return 0;
}

static tissat_job *find_executed_job (pid_t pid) {
  for (all_jobs (job))
    if (job->executed && job->pid == pid)
      return job;
  return 0;
}

static void parallel_progress (unsigned running_jobs) {
  if (!tissat_progress)
    return;
  printf ("parallel: executed %u, finished %u, running %u\n", executed,
          finished, running_jobs);
  fflush (stdout);
}

static void run_jobs_in_parallel (unsigned parallel) {
  if (parallel == UINT_MAX)
    tissat_message ("Running %u jobs in parallel "
                    "using arbitrary many processes.",
                    tissat_scheduled);
  else
    tissat_message ("Running %u jobs in parallel using up to %d processes.",
                    tissat_scheduled, parallel);
  unsigned running = 0;
  while (finished < tissat_scheduled) {
    tissat_job *job;
    if (running < parallel && executed < tissat_scheduled &&
        (job = find_executable_job ())) {
      job->pid = fork ();
      if (job->pid < 0)
        tissat_fatal ("failed to fork job %zu", job->id);
      else if (!job->pid) {
        run_parallel_job (job);
        exit (0);
      } else {
        job->executed = true;
        executed++;
        running++;
        parallel_progress (running);
      }
    } else {
      int status;
      pid_t pid = waitpid (-1, &status, 0);
      if (pid < 0)
        tissat_fatal ("waiting on %u unfinished processes failed",
                      executed - finished);
      tissat_job *other = find_executed_job (pid);
      if (WIFSIGNALED (status))
        handle_signal (other, WTERMSIG (status));
      else if (WIFEXITED (status))
        handle_exit (other, status);
      else
        tissat_fatal ("unexpected status '%d' of child process '%d'",
                      status, pid);
      assert (other);
      other->finished = true;
      finished++;
      assert (running);
      running--;
      parallel_progress (running);
    }
  }
  assert (!running);
}

void tissat_run_jobs (int parallel) {
  if (!parallel)
    run_jobs_sequentially ();
  else
    run_jobs_in_parallel (parallel < 0 ? UINT_MAX : (unsigned) parallel);
}

void tissat_release_jobs (void) {
  for (all_jobs (job)) {
    if (job->command)
      free (job->command);
    if (job->application)
      free (job->application);
    memset (job, 0, sizeof *job);
  }
  tissat_scheduled = 0;
}
