#include "../src/heap.h"
#include "../src/inlineheap.h"

#include "test.h"

static void test_heap_basic (void) {
  assert (DISCONTAINED (DISCONTAIN));
  DECLARE_AND_INIT_SOLVER (solver);
  heap dummy_heap, *heap = &dummy_heap;
  memset (heap, 0, sizeof (struct heap));
  assert (kissat_max_score_on_heap (heap) == 0);
  kissat_resize_heap (solver, heap, 100);
  kissat_push_heap (solver, heap, 0);
  kissat_push_heap (solver, heap, 1);
  assert (kissat_max_heap (heap) == 0);
  kissat_update_heap (solver, heap, 1, 1);
  assert (kissat_max_heap (heap) == 1);
  kissat_push_heap (solver, heap, 2);
  kissat_push_heap (solver, heap, 3);
  kissat_push_heap (solver, heap, 4);
  kissat_push_heap (solver, heap, 5);
  kissat_push_heap (solver, heap, 6);
  kissat_push_heap (solver, heap, 7);
  assert (kissat_max_heap (heap) == 1);
  kissat_update_heap (solver, heap, 2, 2);
  assert (kissat_max_heap (heap) == 2);
  kissat_update_heap (solver, heap, 3, 3);
  assert (kissat_max_heap (heap) == 3);
  kissat_update_heap (solver, heap, 6, 6);
  assert (kissat_max_heap (heap) == 6);
  kissat_update_heap (solver, heap, 4, 4);
  assert (kissat_max_heap (heap) == 6);
  kissat_update_heap (solver, heap, 5, 5);
  assert (kissat_max_heap (heap) == 6);
  kissat_update_heap (solver, heap, 7, 7);
  assert (kissat_max_heap (heap) == 7);
  unsigned count = 7;
  while (!kissat_empty_heap (heap)) {
    unsigned max = kissat_max_heap (heap);
    assert (max == count);
    kissat_pop_heap (solver, heap, max);
    count--;
  }
  kissat_release_heap (solver, heap);
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

static void test_heap_random (void) {
#define M 10
#define N 100
  srand (42);
  for (unsigned i = 0; i < M; i++) {
    DECLARE_AND_INIT_SOLVER (solver);
    heap dummy_heap, *heap = &dummy_heap;
    memset (heap, 0, sizeof (struct heap));
    for (unsigned round = 1; round <= 4; round++) {
      const unsigned n = round * N;
      kissat_resize_heap (solver, heap, n);
      for (unsigned i = 0; i < 2 * n; i++) {
        unsigned idx = rand () % n;
        if (rand () % 3) {
          if (kissat_heap_contains (heap, idx))
            kissat_pop_heap (solver, heap, idx);
          else
            kissat_push_heap (solver, heap, idx);
        } else {
          unsigned score = rand () % (2 * n / 3);
          kissat_update_heap (solver, heap, idx, score);
        }
      }
    }
    kissat_release_heap (solver, heap);
#ifdef METRICS
    assert (!solver->statistics.allocated_current);
#endif
  }
#undef M
#undef N
}

static void test_heap_rescale (void) {
  DECLARE_AND_INIT_SOLVER (solver);
  heap dummy_heap, *heap = &dummy_heap;
  memset (heap, 0, sizeof (struct heap));
  double score = kissat_max_score_on_heap (heap);
  assert (!score);
  kissat_resize_heap (solver, heap, 1);
  kissat_update_heap (solver, heap, 0, 2);
  kissat_push_heap (solver, heap, 0);
  score = kissat_max_score_on_heap (heap);
  assert (score == 2);
  kissat_resize_heap (solver, heap, 3);
  kissat_update_heap (solver, heap, 2, 0);
  score = kissat_max_score_on_heap (heap);
  assert (score == 2);
  kissat_update_heap (solver, heap, 1, 4);
  kissat_push_heap (solver, heap, 2);
  score = kissat_max_score_on_heap (heap);
  assert (score == 4);
  kissat_push_heap (solver, heap, 1);
  score = kissat_max_score_on_heap (heap);
  assert (score == 4);
  kissat_rescale_heap (solver, heap, 0.5);
  score = kissat_max_score_on_heap (heap);
  assert (score == 2);
  kissat_release_heap (solver, heap);
}

void tissat_schedule_heap (void) {
  SCHEDULE_FUNCTION (test_heap_basic);
  SCHEDULE_FUNCTION (test_heap_random);
  SCHEDULE_FUNCTION (test_heap_rescale);
}
