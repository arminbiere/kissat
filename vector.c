#include "allocate.h"
#include "error.h"
#include "collect.h"
#include "internal.h"
#include "logging.h"
#include "print.h"
#include "rank.h"

#include <inttypes.h>

unsigned *
kissat_enlarge_vector (kissat * solver, vectors * vectors, vector * vector)
{
  unsigneds *stack = &vectors->stack;
  LOG2 ("enlarging vector %" SECTOR_FORMAT "[%" SECTOR_FORMAT "] at %p",
	vector->offset, vector->size, vector);
  const sector old_vector_size = vector->size;
  assert (old_vector_size < MAX_VECTORS / 2);
  const sector new_vector_size = old_vector_size ? 2 * old_vector_size : 1;
  size_t old_stack_size = SIZE_STACK (*stack);
  size_t capacity = CAPACITY_STACK (*stack);
  assert (kissat_is_power_of_two (MAX_VECTORS));
  assert (capacity <= MAX_VECTORS);
  size_t available = capacity - old_stack_size;
  if (new_vector_size > available)
    {
#ifndef QUIET
      unsigned *old_begin = BEGIN_STACK (*stack);
#endif
      unsigned enlarged = 0;
      do
	{
	  assert (kissat_is_zero_or_power_of_two (capacity));

	  if (capacity == MAX_VECTORS)
	    kissat_fatal ("maximum vector stack size "
			  "of 2^%u entries %s exhausted", LD_MAX_VECTORS,
			  FORMAT_BYTES (MAX_VECTORS * sizeof (unsigned)));
	  enlarged++;
	  kissat_stack_enlarge (solver, (chars *) stack, sizeof (unsigned));

	  capacity = CAPACITY_STACK (*stack);
	  available = capacity - old_stack_size;
	}
      while (new_vector_size > available);

      if (enlarged)
	{
	  INC (vectors_enlarged);
#ifndef QUIET
	  unsigned *new_begin = BEGIN_STACK (*stack);
	  const uintptr_t moved = new_begin - old_begin;
	  kissat_phase (solver, "vectors",
			GET (vectors_enlarged),
			"enlarged to %s entries %s (%s)",
			FORMAT_COUNT (capacity),
			FORMAT_BYTES (capacity * sizeof (unsigned)),
			(moved ? "moved" : "in place"));
#endif
	}
      assert (capacity <= MAX_VECTORS);
      assert (new_vector_size <= available);
    }
  unsigned *begin_old_vector = kissat_begin_vector (vectors, vector);
  unsigned *begin_new_vector = END_STACK (*stack);
  unsigned *middle_new_vector = begin_new_vector + old_vector_size;
  unsigned *end_new_vector = begin_new_vector + new_vector_size;
  assert (end_new_vector <= stack->allocated);
  const size_t old_bytes = old_vector_size * sizeof (unsigned);
  const size_t delta_size = new_vector_size - old_vector_size;
  const size_t delta_bytes = delta_size * sizeof (unsigned);
  memcpy (begin_new_vector, begin_old_vector, old_bytes);
  memset (begin_old_vector, 0xff, old_bytes);
  solver->vectors.usable += old_vector_size;
  kissat_add_usable (vectors, delta_size);
  memset (middle_new_vector, 0xff, delta_bytes);
  const uint64_t offset = SIZE_STACK (*stack);
  assert (offset <= MAX_VECTORS);
  vector->offset = offset;
  LOG2 ("enlarged vector at %p to %" SECTOR_FORMAT "[%" SECTOR_FORMAT "]",
	vector, vector->offset, vector->size);
  stack->end = end_new_vector;
  assert (begin_new_vector < end_new_vector);
  return middle_new_vector;
}

static inline sector
rank_offset (vector * unsorted, unsigned i)
{
  return unsorted[i].offset;
}

#define RANK_OFFSET(A) \
  rank_offset (unsorted, (A))

#define RADIX_SORT_DEFRAG_LENGTH 16

void
kissat_defrag_vectors (kissat * solver, vectors * vectors,
		       unsigned size_unsorted, vector * unsorted)
{
  START (defrag);
  unsigneds *stack = &vectors->stack;
  const size_t size_vectors = SIZE_STACK (*stack);
  if (size_vectors < 2)
    return;
  INC (defragmentations);
  LOG ("defragmenting vectors size %zu capacity %" PRIu64
       " usable %" SECTOR_FORMAT,
       size_vectors, CAPACITY_STACK (*stack), solver->vectors.usable);
  size_t bytes = size_unsorted * sizeof (unsigned);
  unsigned *sorted = kissat_malloc (solver, bytes);
  unsigned size_sorted = 0;
  for (unsigned i = 0; i < size_unsorted; i++)
    {
      vector *vector = unsorted + i;
      if (vector->size)
	sorted[size_sorted++] = i;
      else
	vector->offset = 0;
    }
  RADIX (RADIX_SORT_DEFRAG_LENGTH,
	 unsigned, sector, size_sorted, sorted, RANK_OFFSET);
  unsigned *begin = BEGIN_STACK (*stack);
  unsigned *p = begin + 1;
  for (unsigned i = 0; i < size_sorted; i++)
    {
      unsigned j = sorted[i];
      vector *vector = unsorted + j;
      const sector old_offset = vector->offset;
      const sector size = vector->size;
      const sector new_offset = p - begin;
      assert (new_offset <= old_offset);
      vector->offset = new_offset;
      const unsigned *q = begin + old_offset;
      memmove (p, q, size * sizeof (unsigned));
      p += size;
    }
  kissat_free (solver, sorted, bytes);
#ifndef QUIET
  const size_t freed = END_STACK (*stack) - p;
  double freed_fraction = kissat_percent (freed, size_vectors);
  kissat_phase (solver, "defrag", GET (defragmentations),
		"freed %zu usable entries %.0f%%", freed, freed_fraction);
  assert (freed == solver->vectors.usable);
#endif
  SET_END_OF_STACK (*stack, p);
  SHRINK_STACK (*stack);
  solver->vectors.usable = 0;
  kissat_check_vectors (solver);
  STOP (defrag);
}

void
kissat_remove_from_vector (kissat * solver,
			   vectors * vectors, vector * vector,
			   unsigned remove)
{
  unsigned *begin = kissat_begin_vector (vectors, vector), *p = begin;
  const unsigned *end = kissat_end_vector (vectors, vector);
  assert (p != end);
  while (*p != remove)
    p++, assert (p != end);
  while (++p != end)
    p[-1] = *p;
  p[-1] = INVALID_VECTOR_ELEMENT;
  vector->size--;
  kissat_inc_usable (vectors);
  kissat_check_vectors (solver);
#ifndef CHECK_VECTORS
  (void) solver;
#endif
}

void
kissat_resize_vector (kissat * solver, vectors * vectors, vector * vector,
		      sector new_size)
{
  const sector old_size = vector->size;
  assert (new_size <= old_size);
  if (new_size == old_size)
    return;
  vector->size = new_size;
  unsigned *begin = kissat_begin_vector (vectors, vector);
  unsigned *end = begin + new_size;
  size_t delta = old_size - new_size;
  kissat_add_usable (vectors, delta);
  size_t bytes = delta * sizeof (unsigned);
  memset (end, 0xff, bytes);
  kissat_check_vectors (solver);
#ifndef CHECK_VECTORS
  (void) solver;
#endif
}

void
kissat_release_vector (kissat * solver, vectors * vectors, vector * vector)
{
  kissat_resize_vector (solver, vectors, vector, 0);
}

#ifdef CHECK_VECTORS

void
kissat_check_vector (vectors * vectors, vector * vector)
{
  const unsigned *begin = kissat_begin_vector (vectors, vector);
  const unsigned *end = kissat_end_vector (vectors, vector);
  for (const unsigned *p = begin; p != end; p++)
    assert (*p != INVALID_VECTOR_ELEMENT);
}

void
kissat_check_vectors (kissat * solver)
{
  for (all_literals (lit))
    {
      vector *vector = &WATCHES (lit);
      kissat_check_vector (&solver->vectors, vector);
    }
  vectors *vectors = &solver->vectors;
  unsigneds *stack = &vectors->stack;
  const unsigned *begin = BEGIN_STACK (*stack);
  const unsigned *end = END_STACK (*stack);
  if (begin == end)
    return;
  sector invalid = 0;
  for (const unsigned *p = begin + 1; p != end; p++)
    if (*p == INVALID_VECTOR_ELEMENT)
      invalid++;
  assert (invalid == solver->vectors.usable);
}

#endif
