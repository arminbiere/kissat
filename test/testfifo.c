#include "../src/allocate.h"
#include "../src/fifo.h"

#include "test.h"

static void test_fifo_basic (void) {
  DECLARE_AND_INIT_SOLVER (solver);
  FIFO (unsigned) fifo;
  assert (sizeof fifo == 5 * sizeof (void *));
  INIT_FIFO (fifo);
  for (unsigned i = 0; i != 20; i++) {
    printf ("enqueue %u\n", i);
    ENQUEUE_FIFO (fifo, i);
  }
  for (unsigned i = 0, j; i != 10; i++) {
    DEQUEUE_FIFO (fifo, j);
    printf ("dequeue %u\n", j);
  }
  for (unsigned i = 20, j; i != 40; i++) {
    printf ("enqueue %u\n", i);
    ENQUEUE_FIFO (fifo, i);
    DEQUEUE_FIFO (fifo, j);
    printf ("dequeue %u\n", j);
  }
  size_t size = SIZE_FIFO (fifo);
  printf ("size %zu\n", size);
  size_t capacity = CAPACITY_FIFO (fifo);
  printf ("capacity %zu\n", capacity);
  unsigned j = 0;
  for (all_fifo (unsigned, i, fifo))
    printf ("fifo[%u] = %u\n", j++, i);
  assert (j == size);
  while (!EMPTY_FIFO (fifo)) {
    DEQUEUE_FIFO (fifo, j);
    printf ("dequeue %u\n", j);
  }
  RELEASE_FIFO (fifo);
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

void tissat_schedule_fifo (void) { SCHEDULE_FUNCTION (test_fifo_basic); }
