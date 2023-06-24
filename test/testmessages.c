#include "../src/colors.h"
#include "../src/handle.h"

#include <signal.h>
#include <stdarg.h>

#include "test.h"

int tissat_verbosity;
int tissat_warnings;

void tissat_message (const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

void tissat_verbose (const char *fmt, ...) {
  if (!tissat_verbosity)
    return;
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

void tissat_bold_message (const char *fmt, ...) {
  va_list ap;
  TERMINAL (stdout, 1);
  COLOR (BOLD);
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  COLOR (NORMAL);
  fputc ('\n', stdout);
  fflush (stdout);
}

void tissat_warning (const char *fmt, ...) {
  va_list ap;
  TERMINAL (stdout, 1);
  COLOR (BOLD YELLOW);
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  COLOR (NORMAL);
  fputc ('\n', stdout);
  fflush (stdout);
  tissat_warnings++;
}

void tissat_line (void) {
  fputc ('\n', stdout);
  fflush (stdout);
}

static void tissat_start_fatal_or_error_message (const char *type) {
  fflush (stdout);
  TERMINAL (stderr, 2);
  COLOR (BOLD);
  fputs ("tissat: ", stderr);
  COLOR (RED);
  fputs (type, stderr);
  fputs (": ", stderr);
  COLOR (NORMAL);
}

void tissat_error (const char *fmt, ...) {
  tissat_start_fatal_or_error_message ("error");
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
  exit (1);
}

void tissat_fatal (const char *fmt, ...) {
  tissat_start_fatal_or_error_message ("fatal error");
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
  abort ();
}

void tissat_signal (int sig, const char *fmt, ...) {
  tissat_start_fatal_or_error_message ("unexpected signal");
  fprintf (stderr, "Caught signal '%d' (%s) ", sig,
           kissat_signal_name (sig));
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
  exit (1);
}

void tissat_section (const char *fmt, ...) {
  if (!tissat_verbosity)
    return;
  fputc ('\n', stdout);
  char buffer[1024];
  va_list ap;
  va_start (ap, fmt);
  vsprintf (buffer, fmt, ap);
  va_end (ap);
  TERMINAL (stdout, 1);
  COLOR (BOLD YELLOW);
  fputs ("====== [ ", stdout);
  fputs (buffer, stdout);
  fputs (" ] ", stdout);
  const size_t len = strlen (buffer);
  for (size_t i = len; i < 66; i++)
    fputc ('=', stdout);
  COLOR (NORMAL);
  fputs ("\n\n", stdout);
  fflush (stdout);
}
