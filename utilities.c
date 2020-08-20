#include "utilities.h"

#include <string.h>

bool
kissat_has_suffix (const char *str, const char *suffix)
{
  size_t l = strlen (str);
  size_t k = strlen (suffix);
  if (l < k)
    return false;
  return !strcmp (str + l - k, suffix);
}

unsigned
kissat_ldceil (word w)
{
  unsigned res = 0;
  word limit = 1;
  while (limit && w > limit)
    {
      limit <<= 1;
      res++;
    }
  return res;
}
