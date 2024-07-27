#include "../src/inlinequeue.h"

#include "test.h"

static void print_queue (queue *queue, links *links) {
  if (!tissat_verbosity)
    return;
  for (int i = queue->first; i >= 0; i = links[i].next)
    printf ("%d[%u] ", i, links[i].stamp);
  printf ("search %u\n", queue->search.idx);
  fflush (stdout);
}

static void test_queue (void) {
  assert (DISCONNECTED (DISCONNECT));
#define size 16
  links links[size];
  value values[2 * size];
  DECLARE_AND_INIT_SOLVER (solver);
  for (unsigned idx = 0; idx < size; idx++) {
    const unsigned lit = 2 * idx;
    const unsigned not_lit = lit + 1;
    switch (idx % 3) {
    default:
    case 0:
      values[lit] = 0;
      values[not_lit] = 0;
      break;
    case 1:
      values[lit] = 1;
      values[not_lit] = -1;
      break;
    case 2:
      values[lit] = -1;
      values[not_lit] = 1;
      break;
    }
  }
  solver->values = values;
  solver->links = links;
  solver->vars = size;
  queue *queue = &solver->queue;
  kissat_init_queue (solver);
  for (int i = 0; i < size; i++)
    kissat_enqueue (solver, i);
  int c = 0;
  for (int i = queue->first; i >= 0; i = links[i].next, c++)
    assert (i == c);
  kissat_move_to_front (solver, queue->last);
  print_queue (queue, links);
  for (int i = 0; i < size; i += 2)
    kissat_move_to_front (solver, i);
  print_queue (queue, links);
  unsigned search = queue->search.idx;
  assert (search != queue->last);
  assert (!values[2 * search]);
  assert (!values[2 * search + 1]);
  values[2 * search] = 1;
  values[2 * search + 1] = -1;
  kissat_move_to_front (solver, search);
  print_queue (queue, links);
  for (unsigned idx = 0; idx < size; idx++) {
    const unsigned lit = 2 * idx;
    const unsigned not_lit = lit + 1;
    values[lit] = 1;
    values[not_lit] = -1;
  }
  c = 1;
  for (int i = 1; i < size; i += 2) {
    assert (i == c);
    c += 2;
    if (c >= size)
      c = 0;
  }
  for (int i = 1; i < size; i += 2)
    kissat_move_to_front (solver, i);
  print_queue (queue, links);
  queue->stamp = queue->search.stamp = links[queue->last].stamp = UINT_MAX;
  print_queue (queue, links);
  for (int i = size - 1; i >= 0; i--)
    kissat_move_to_front (solver, i);
  print_queue (queue, links);
  c = size - 1;
  for (int i = queue->first; i >= 0; i = links[i].next, c--)
    assert (i == c);
#undef size
}

void tissat_schedule_queue (void) { SCHEDULE_FUNCTION (test_queue); }
