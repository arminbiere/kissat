#include "test.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int dev_null = -1;
static int saved[3] = { -1, -1, -1 };

void
tissat_divert_stdout_and_stderr_to_dev_null (void)
{
  if (tissat_verbosity)
    return;
  assert (saved[1] < 0);
  assert (saved[2] < 0);
  if (dev_null < 0)
    dev_null = open ("/dev/null", O_WRONLY);
  assert (dev_null >= 3);
  saved[1] = dup (1);
  assert (saved[1] >= 4);
  saved[2] = dup (2);
  assert (saved[2] >= 5);
  int res;
  res = dup2 (dev_null, 1);
  assert (res == 1);
  res = dup2 (dev_null, 2);
  assert (res == 2);
}

#define ASSERT(COND) \
do { \
  if (COND) \
    break; \
  abort (); \
} while (true)

static void
restore_stdout (void)
{
  if (saved[1] < 0)
    return;
  fflush (stdout);
  ASSERT (saved[1] >= 4);
  int res;
  res = dup2 (saved[1], 1);
  ASSERT (res == 1);
  res = close (saved[1]);
  ASSERT (!res);
  saved[1] = -1;
}

static void
restore_stderr (void)
{
  if (saved[2] < 0)
    return;
  fflush (stderr);
  ASSERT (saved[2] >= 5);
  int res;
  res = dup2 (saved[2], 2);
  ASSERT (res == 2);
  res = close (saved[2]);
  ASSERT (!res);
  saved[2] = -1;
}

void
tissat_restore_stdout_and_stderr (void)
{
  if (tissat_verbosity)
    return;
  restore_stdout ();
  restore_stderr ();
}

void
tissat_redirect_stderr_to_stdout (void)
{
  if (dev_null >= 0)
    return;
  assert (saved[2] < 0);
  saved[2] = dup (2);
  assert (saved[2] >= 3);
  int res;
  res = dup2 (1, 2);
  assert (res == 2);
}

void
tissat_restore_stderr (void)
{
  if (dev_null >= 0)
    return;
  if (saved[2] < 0)
    return;
  fflush (stderr);
  ASSERT (saved[2] >= 3);
  int res;
  res = dup2 (saved[2], 2);
  ASSERT (res == 2);
  res = close (saved[2]);
  ASSERT (!res);
  saved[2] = -1;
}
