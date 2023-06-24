#include "test.h"

bool test_sizes (void) {
  printf ("sizeof (word) = %zu\n", sizeof (word));
  assert (sizeof (void *) == sizeof (word));
  assert (MAX_REF < INVALID_REF);
  assert (MAX_ARENA < INVALID_REF);
  printf ("sizeof (clause) = %zu\n", sizeof (clause));
  printf ("SIZE_OF_CLAUSE_HEADER = %zu\n", SIZE_OF_CLAUSE_HEADER);
  assert (SIZE_OF_CLAUSE_HEADER == sizeof (unsigned));
  printf ("sizeof (flags) = %zu\n", sizeof (flags));
  assert (sizeof (flags) == 1);
  printf ("sizeof (value) = %zu\n", sizeof (value));
  assert (sizeof (value) == 1);
  return false;
}
