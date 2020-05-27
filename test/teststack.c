#include "test.h"

#include "../src/allocate.h"

static void
test_stack_basic (void)
{
  DECLARE_AND_INIT_SOLVER (solver);
  STACK (unsigned) stack;
  assert (sizeof stack == 3 * sizeof (void *));
  INIT_STACK (stack);
  assert (EMPTY_STACK (stack));
  assert (FULL_STACK (stack));
  const unsigned n = 100;
  for (unsigned i = 0; i < n; i++)
    {
      assert (SIZE_STACK (stack) == i);
      PUSH_STACK (stack, i);
    }
#ifndef NMETRICS
  assert (solver->statistics.allocated_current == 128 * sizeof (unsigned));
#endif
  {
    unsigned i = 0;
    for (all_stack (unsigned, e, stack))
        assert (e == i++);
    assert (i == n);
  }
  {
    unsigned i = n - 1;
    while (!EMPTY_STACK (stack))
      {
	unsigned tmp = TOP_STACK (stack);
	assert (tmp == i);
	tmp = POP_STACK (stack);
	assert (tmp == i);
	i--;
      }
    assert (i == 0u - 1);
  }
  RELEASE_STACK (stack);
#ifndef NMETRICS
  assert (!solver->statistics.allocated_current);
#endif
}

typedef struct odd_sized odd_sized;

struct odd_sized
{
  unsigned a, b, c;
};

// *INDENT-OFF*
typedef STACK (odd_sized) odd_sized_stack;
// *INDENT-ON*

static void
test_shrink_stack (void)
{
  DECLARE_AND_INIT_SOLVER (solver);
  odd_sized element;
  memset (&element, 0, sizeof element);
  odd_sized_stack stack;
  INIT_STACK (stack);
  PUSH_STACK (stack, element);
  PUSH_STACK (stack, element);
  PUSH_STACK (stack, element);
  PUSH_STACK (stack, element);
  RESIZE_STACK (stack, 1);
  SHRINK_STACK (stack);
  RELEASE_STACK (stack);
#ifndef NMETRICS
  assert (!solver->statistics.allocated_current);
#endif
}

void
tissat_schedule_stack (void)
{
  SCHEDULE_FUNCTION (test_stack_basic);
  SCHEDULE_FUNCTION (test_shrink_stack);
}
