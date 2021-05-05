#include "build.h"
#include "colors.h"
#include "print.h"
#include "kissat.h"

#include <stdio.h>

const char *
kissat_signature (void)
{
  return "kissat-" VERSION;
}

const char *
kissat_id (void)
{
  return ID;
}

const char *
kissat_compiler (void)
{
  return COMPILER;
}

const char *
kissat_copyright (void)
{
  return "Copyright (c) 2019-2021 Armin Biere JKU Linz";
}

const char *
kissat_version (void)
{
  return VERSION;
}

#define PREFIX(COLORS) \
do { \
  if (prefix) \
    fputs (prefix, stdout); \
  COLOR (COLORS); \
} while (0)

#define NL() \
do { \
  fputs ("\n", stdout); \
  COLOR (NORMAL); \
} while (0)

void
kissat_build (const char *prefix)
{
  TERMINAL (stdout, 1);
  if (!prefix)
    connected_to_terminal = false;

  PREFIX (MAGENTA);
  if (ID)
    printf ("Version %s %s", VERSION, ID);
  else
    printf ("Version %s", VERSION);
  NL ();

  PREFIX (MAGENTA);
  printf ("%s", COMPILER);
  NL ();

  PREFIX (MAGENTA);
  printf ("%s", BUILD);
  NL ();
}

void
kissat_banner (const char *prefix, const char *name)
{
  TERMINAL (stdout, 1);
  if (!prefix)
    connected_to_terminal = false;

  PREFIX (BOLD MAGENTA);
  printf ("%s", name);
  NL ();

  PREFIX (BOLD MAGENTA);
  fputs (kissat_copyright (), stdout);
  NL ();

  if (prefix)
    {
      PREFIX ("");
      NL ();
    }

  kissat_build (prefix);
}
