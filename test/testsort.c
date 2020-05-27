#include  "test.h"

#include  "../src/allocate.h"
#include  "../src/sort.h"

static bool
less_unsigned (unsigned a, unsigned b)
{
  return a < b;
}

static void
test_sort_unsigneds (void)
{
  DECLARE_AND_INIT_SOLVER (solver);
#define N 20
  unsigned found[N];
  memset (found, 0, sizeof found);
  unsigneds stack;
  INIT_STACK (stack);
  SORT_STACK (unsigned, stack, less_unsigned);
  srand (42);
  for (unsigned i = 0; i < N; i++)
    {
      unsigned tmp = rand () % N / 2;
      PUSH_STACK (stack, tmp);
      found[tmp]++;
    }
  SORT_STACK (unsigned, stack, less_unsigned);
  for (all_stack (unsigned, i, stack))
      printf ("%u\n", i);
  for (all_stack (unsigned, i, stack))
    {
      assert (found[i]);
      found[i]--;
    }
  for (unsigned i = 0; i < N; i++)
    assert (!found[i]);
  for (unsigned i = 1; i < N; i++)
    assert (PEEK_STACK (stack, i - 1) <= PEEK_STACK (stack, i));
  RELEASE_STACK (stack);
  RELEASE_STACK (SORTER);
#ifndef QUIET
  RELEASE_STACK (solver->profiles.stack);
#endif
#ifndef NMETRICS
  assert (!solver->statistics.allocated_current);
#endif
#undef N
}

static bool
less_str (const char *a, const char *b)
{
  return strcmp (a, b) < 0;
}

static void
test_sort_strings (void)
{
  struct kissat dummy, *solver = &dummy;
  memset (&dummy, 0, sizeof dummy);
  STACK (const char *) stack;
  INIT_STACK (stack);
  SORT_STACK (const char *, stack, less_str);
  PUSH_STACK (stack, "zzzzz");
  SORT_STACK (const char *, stack, less_str);
  PUSH_STACK (stack, "ccccc");
  PUSH_STACK (stack, "bbbbb");
  PUSH_STACK (stack, "xxxxx");
  PUSH_STACK (stack, "aaaaa");
  SORT_STACK (const char *, stack, less_str);
  for (all_pointers (const char, s, stack))
      printf ("%s\n", s);
  RELEASE_STACK (stack);
  RELEASE_STACK (SORTER);
#ifndef QUIET
  RELEASE_STACK (solver->profiles.stack);
#endif
#ifndef NMETRICS
  assert (!solver->statistics.allocated_current);
#endif
}

void
tissat_schedule_sort (void)
{
  SCHEDULE_FUNCTION (test_sort_unsigneds);
  SCHEDULE_FUNCTION (test_sort_strings);
}
