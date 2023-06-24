#include "test.h"

#include "../src/array.h"

typedef struct pair pair;

struct pair {
  unsigned first;
  unsigned second;
};

static void test_array_basic (void) {
  DECLARE_AND_INIT_SOLVER (solver);
  ARRAY (pair) array;
  memset (&array, 0, sizeof array);
  assert (EMPTY_ARRAY (array));
  size_t size = 0;
  unsigned sum = 0;
  for (unsigned l = 0; l <= 10; l++) {
    size_t new_size = 1u << l;
    REALLOCATE_ARRAY (array, size, new_size);
    for (unsigned i = 0; i < size; i++) {
      pair pair = PEEK_ARRAY (array, i);
      assert (pair.first == i);
      assert (pair.second == ~i);
    }
    for (unsigned i = size; i < new_size; i++) {
      pair pair;
      pair.first = i;
      pair.second = ~i;
      PUSH_ARRAY (array, pair);
      sum += i;
    }
    assert (!EMPTY_ARRAY (array));
    assert (SIZE_ARRAY (array) == new_size);
    size = new_size;
  }
  pair *a = BEGIN_ARRAY (array);
  for (unsigned i = 0; i < size; i++) {
    pair *p = &a[i];
    SWAP (unsigned, p->first, p->second);
  }
  for (all_stack (pair, pair, array))
    sum -= pair.second;
  assert (!sum);
  for (unsigned expect = size - 1; !EMPTY_ARRAY (array); expect--) {
    pair pair = POP_ARRAY (array);
    assert (pair.first == ~expect);
    assert (pair.second == expect);
  }
  RELEASE_ARRAY (array, size);
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

void tissat_schedule_array (void) { SCHEDULE_FUNCTION (test_array_basic); }
