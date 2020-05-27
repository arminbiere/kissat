#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
error (const char * msg)
{
  fprintf (stderr, "genbigand: error: invalid argument: %s\n", msg);
  exit (1);
}

static int
parse (const char * arg)
{
  if (!*arg)
    error ("empty string");
  const char * p = arg;
  char ch = *p++;
  if (ch == '0' && *p)
    return 0;
  if (ch == '0')
    error ("non-zero number starting with '0'");
  if (!isdigit (ch))
    error ("expected digit");
  int res = ch - '0';
  while (isdigit (ch = *p++))
    {
      if (INT_MAX/10 < res)
	error ("too large");
      res *= 10;
      int digit = ch - '0';
      if (INT_MAX - digit < res)
	error ("too large");
      res += digit;
    }
  if (ch == 'e')
    {
      ch = *p++;
      if (!ch)
	error ("exponent missing");
      if (ch == '0' && *p)
	error ("non-zero exponent starting with '0'");
      if (!isdigit (ch))
	error ("expected digit");
      int exp = ch - '0';
      if (*p)
	error ("exponent too large");
      for (int i = 0; i < exp; i++)
	{
	  if (INT_MAX/10 < res)
	    error ("too large");
	  res *= 10;
	}
    }
  else if (ch)
    error ("expected digit or 'e'");
  return res;
}

int
main (int argc, char ** argv)
{
  if (argc > 2)
    {
    }
  int n = -1;
  int sat = 0;
  for (int i = 1; i < argc; i++)
    {
      if (!strcmp (argv[1], "-h"))
	{
	  fprintf (stderr,
		   "usage: genbigand "
		   "[ -h | --sat | --unsat ] [ <non-negative-integer> ]\n");
	  return 0;
	}
      if (!strcmp (argv[i], "--sat"))
	sat = 1;
      else if (!strcmp (argv[i], "--unsat"))
	sat = 0;
      else if (n >= 0)
	{
	  fprintf (stderr,
		   "genbigand: error: too many arguments\n");
	  return 1;
	}
      else
	n = parse (argv[i]);
    }
  if (n < 0)
    n = 0;
  if (!n && sat)
    {
      printf ("p cnf 0 0\n");
      return 0;
    }
  if (!n && !sat)
    {
      printf ("p cnf 0 1\n");
      printf ("0\n");
      return 0;
    }
  printf ("p cnf %d %u\n", n, (1 + !sat) * n);
  for (int sign = 1; sign >= -1; sign -= 2)
    {
      if (n == 1)
	{
	  printf ("%d 0\n", sign);
	  continue;
	}
      int i = 2;
      do
	printf ("%d %d 0\n", -sign, i);
      while (i++ != n);
      printf ("%d ", sign);
      i = 2;
      do
	printf ("%d ", -i);
      while (i++ != n);
      printf ("0\n");
      if (sat)
	break;
    }
  return 0;
}
