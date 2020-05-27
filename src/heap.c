#include "allocate.h"
#include "internal.h"
#include "logging.h"

#include <string.h>

void
kissat_release_heap (kissat * solver, heap * heap)
{
  RELEASE_STACK (heap->stack);
  DEALLOC (heap->pos, heap->size);
  DEALLOC (heap->score, heap->size);
  memset (heap, 0, sizeof *heap);
}

#define CHILD(POS) (2*(POS) + 1)
#define PARENT(POS) (((POS) - 1)/2)

#ifndef NDEBUG

void
kissat_check_heap (heap * heap)
{
  const unsigned *stack = BEGIN_STACK (heap->stack);
  const unsigned end = SIZE_STACK (heap->stack);
  const unsigned *pos = heap->pos;
  const double *score = heap->score;
  for (unsigned i = 0; i < end; i++)
    {
      const unsigned idx = stack[i];
      const unsigned idx_pos = pos[idx];
      assert (idx_pos == i);
      unsigned child_pos = CHILD (idx_pos);
      unsigned parent_pos = PARENT (child_pos);
      assert (parent_pos == idx_pos);
      if (child_pos < end)
	{
	  unsigned child = stack[child_pos];
	  assert (score[idx] >= score[child]);
	  if (++child_pos < end)
	    {
	      parent_pos = PARENT (child_pos);
	      assert (parent_pos == idx_pos);
	      child = stack[child_pos];
	      assert (score[idx] >= score[child]);
	    }
	}
    }
}

#endif

void
kissat_resize_heap (kissat * solver, heap * heap, unsigned new_size)
{
  const unsigned old_size = heap->size;
  if (old_size >= new_size)
    return;
  LOG ("resizing %s heap from %u to %u",
       (heap->tainted ? "tainted" : "untainted"), old_size, new_size);

  heap->pos = kissat_nrealloc (solver, heap->pos,
			       old_size, new_size, sizeof (unsigned));
  if (heap->tainted)
    {
      heap->score = kissat_nrealloc (solver, heap->score,
				     old_size, new_size, sizeof (double));
    }
  else
    {
      if (old_size)
	DEALLOC (heap->score, old_size);
      heap->score = kissat_calloc (solver, new_size, sizeof (double));
    }
  heap->size = new_size;
#ifdef CHECK_HEAP
  kissat_check_heap (heap);
#endif
}

static void
bubble_up (kissat * solver, heap * heap, unsigned idx)
{
  unsigned *stack = BEGIN_STACK (heap->stack);
  unsigned *pos = heap->pos;
  unsigned idx_pos = pos[idx];
  const double *score = heap->score;
  const double idx_score = score[idx];
  while (idx_pos)
    {
      const unsigned parent_pos = PARENT (idx_pos);
      const unsigned parent = stack[parent_pos];
      if (score[parent] >= idx_score)
	break;
      LOG ("heap bubble up: %u@%u = %g swapped with %u@%u = %g",
	   parent, parent_pos, score[parent], idx, idx_pos, idx_score);
      stack[idx_pos] = parent;
      pos[parent] = idx_pos;
      idx_pos = parent_pos;
    }
  stack[idx_pos] = idx;
  pos[idx] = idx_pos;
#ifndef LOGGING
  (void) solver;
#endif
}

static void
bubble_down (kissat * solver, heap * heap, unsigned idx)
{
  unsigned *stack = BEGIN_STACK (heap->stack);
  const unsigned end = SIZE_STACK (heap->stack);
  unsigned *pos = heap->pos;
  unsigned idx_pos = pos[idx];
  const double *score = heap->score;
  const double idx_score = score[idx];
  for (;;)
    {
      unsigned child_pos = CHILD (idx_pos);
      if (child_pos >= end)
	break;
      unsigned child = stack[child_pos];
      double child_score = score[child];
      const unsigned sibling_pos = child_pos + 1;
      if (sibling_pos < end)
	{
	  const unsigned sibling = stack[sibling_pos];
	  const double sibling_score = score[sibling];
	  if (sibling_score > child_score)
	    {
	      child = sibling;
	      child_pos = sibling_pos;
	      child_score = sibling_score;
	    }
	}
      if (child_score <= idx_score)
	break;
      LOG ("heap bubble down: %u@%u = %g swapped with %u@%u = %g",
	   child, child_pos, score[child], idx, idx_pos, idx_score);
      stack[idx_pos] = child;
      pos[child] = idx_pos;
      idx_pos = child_pos;
    }
  stack[idx_pos] = idx;
  pos[idx] = idx_pos;
#ifndef LOGGING
  (void) solver;
#endif
}

static void
enlarge_heap (kissat * solver, heap * heap, unsigned new_vars)
{
  const unsigned old_vars = heap->vars;
  assert (old_vars < new_vars);
  assert (new_vars <= heap->size);
  const size_t delta = new_vars - heap->vars;
  memset (heap->pos + old_vars, 0xff, delta * sizeof (unsigned));
  heap->vars = new_vars;
  if (heap->tainted)
    memset (heap->score + old_vars, 0, delta * sizeof (double));
  LOG ("enlarged heap from %u to %u", old_vars, new_vars);
#ifndef LOGGING
  (void) solver;
#endif
}

#define IMPORT(IDX) \
do { \
  assert ((IDX) < UINT_MAX-1); \
  if (heap->vars <= (IDX)) \
    enlarge_heap (solver, heap, (IDX) + 1); \
} while (0)

void
kissat_push_heap (kissat * solver, heap * heap, unsigned idx)
{
  LOG ("push heap %u", idx);
  assert (!kissat_heap_contains (heap, idx));
  IMPORT (idx);
  heap->pos[idx] = SIZE_STACK (heap->stack);
  PUSH_STACK (heap->stack, idx);
  bubble_up (solver, heap, idx);
}

void
kissat_pop_heap (kissat * solver, heap * heap, unsigned idx)
{
  LOG ("pop heap %u", idx);
  assert (kissat_heap_contains (heap, idx));
  IMPORT (idx);
  const unsigned last = POP_STACK (heap->stack);
  heap->pos[last] = DISCONTAIN;
  if (last == idx)
    return;
  const unsigned idx_pos = heap->pos[idx];
  heap->pos[idx] = DISCONTAIN;
  POKE_STACK (heap->stack, idx_pos, last);
  heap->pos[last] = idx_pos;
  bubble_up (solver, heap, last);
  bubble_down (solver, heap, last);
#ifdef CHECK_HEAP
  kissat_check_heap (heap);
#endif
}

void
kissat_update_heap (kissat * solver, heap * heap,
		    unsigned idx, double new_score)
{
  const double old_score = kissat_get_heap_score (heap, idx);
  if (old_score == new_score)
    return;
  IMPORT (idx);
  LOG ("update heap %u score from %g to %g", idx, old_score, new_score);
  heap->score[idx] = new_score;
  if (!heap->tainted)
    {
      heap->tainted = true;
      LOG ("tainted heap");
    }
  if (!kissat_heap_contains (heap, idx))
    return;
  if (new_score > old_score)
    bubble_up (solver, heap, idx);
  else
    bubble_down (solver, heap, idx);
#ifdef CHECK_HEAP
  kissat_check_heap (heap);
#endif
}

double
kissat_max_score_on_heap (heap * heap)
{
  if (!heap->tainted)
    return 0;
  assert (heap->vars);
  const double *score = heap->score;
  const double *end = score + heap->vars;
  double res = score[0];
  for (const double *p = score + 1; p != end; p++)
    res = MAX (res, *p);
  return res;
}

void
kissat_rescale_heap (kissat * solver, heap * heap, double factor)
{
  LOG ("rescaling scores on heap with factor %g", factor);
  double *score = heap->score;
  for (unsigned i = 0; i < heap->vars; i++)
    score[i] *= factor;
#ifndef NDEBUG
  kissat_check_heap (heap);
#endif
#ifndef LOGGING
  (void) solver;
#endif
}

#ifndef NDEBUG

static void
dump_heap (heap * heap)
{
  for (unsigned i = 0; i < SIZE_STACK (heap->stack); i++)
    printf ("heap.stack[%u] = %u\n", i, PEEK_STACK (heap->stack, i));
  for (unsigned i = 0; i < heap->vars; i++)
    printf ("heap.pos[%u] = %u\n", i, heap->pos[i]);
  for (unsigned i = 0; i < heap->vars; i++)
    printf ("heap.score[%u] = %g\n", i, heap->score[i]);
}

void
kissat_dump_heap (heap * heap)
{
  dump_heap (heap);
}

#endif
