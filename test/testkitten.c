#ifdef KITTEN

#include "../src/file.h"

#include "test.h"
#include "testcnfs.h"

static void
schedule_solve_kitten (int expected, const char *kitten, const char *name)
{
  char *cmd = malloc (strlen (kitten) + strlen (name) + 32);
  sprintf (cmd, "%s ../test/cnf/%s.cnf", kitten, name);
  tissat_schedule_command (expected, cmd, 0);
  free (cmd);
}

static void
schedule_solving_kitten (const char *kitten)
{
#define CNF(EXPECTED,NAME,BIG) \
  if (!BIG || tissat_big) \
    schedule_solve_kitten (EXPECTED, kitten, #NAME);
  CNFS
#undef CNF
}

static void
schedule_core_kitten (const char *kitten, const char *name)
{
  char *cmd = malloc (2 * strlen (kitten) + 2 * strlen (name) + 64);
  sprintf (cmd, "%s -O2 ../test/cnf/%s.cnf %s/%s.core",
	   kitten, name, tissat_root, name);
  tissat_job *reduce = tissat_schedule_command (20, cmd, 0);
  sprintf (cmd, "%s %s/%s.core", kitten, tissat_root, name);
  tissat_schedule_command (20, cmd, reduce);
  free (cmd);
}

static void
schedule_cores_kitten (const char *kitten)
{
#define CNF(EXPECTED,NAME,BIG) \
  if (EXPECTED == 20 && (!BIG || tissat_big)) \
    schedule_core_kitten (kitten, #NAME);
  CNFS
#undef CNF
}

#endif

void
tissat_schedule_kitten (void)
{
#ifdef KITTEN
  char *kitten = malloc (strlen (tissat_root) + 16);
  sprintf (kitten, "%s/kitten", tissat_root);
  if (kissat_file_readable (kitten))
    {
      schedule_solving_kitten (kitten);
      schedule_cores_kitten (kitten);
    }
  free (kitten);
#endif
}
