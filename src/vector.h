#ifndef _vector_h_INCLUDED
#define _vector_h_INCLUDED

#include "stack.h"
#include "utilities.h"

#include <limits.h>
#include <string.h>

#ifdef COMPACT
typedef unsigned sector;
#define SECTOR_FORMAT "u"
#else
#include "utilities.h"
typedef word sector;
#define SECTOR_FORMAT WORD_FORMAT
#endif

#define LD_MAX_VECTORS \
  (sizeof (sector) == 8 ? 48 : sizeof (word) == 8 ? 32 : 28)

#define MAX_VECTORS (((uint64_t) 1) << LD_MAX_VECTORS)

#define INVALID_VECTOR_ELEMENT UINT_MAX

#define MAX_SECTOR (~(sector)0)

typedef struct vector vector;
typedef struct vectors vectors;

struct vectors
{
  unsigneds stack;
  sector usable;
};

struct vector
{
  sector offset;
  sector size;
};

struct kissat;

#ifdef CHECK_VECTORS
void kissat_check_vectors (struct kissat *);
#else
#define kissat_check_vectors(...) do { } while (0)
#endif

static inline unsigned *
kissat_begin_vector (vectors * vectors, vector * vector)
{
  return BEGIN_STACK (vectors->stack) + vector->offset;
}

static inline unsigned *
kissat_end_vector (vectors * vectors, vector * vector)
{
  return kissat_begin_vector (vectors, vector) + vector->size;
}

static inline void
kissat_inc_usable (vectors * vectors)
{
  assert (MAX_SECTOR > vectors->usable);
  vectors->usable++;
}

static inline void
kissat_add_usable (vectors * vectors, sector inc)
{
  assert (MAX_SECTOR - inc >= vectors->usable);
  vectors->usable += inc;
}

static inline unsigned *
kissat_last_vector_pointer (vectors * vectors, vector * vector)
{
  assert (vector->size);
  unsigned *begin = kissat_begin_vector (vectors, vector);
  return begin + vector->size - 1;
}

static inline void
kissat_pop_vector (vectors * vectors, vector * vector)
{
  unsigned *p = kissat_last_vector_pointer (vectors, vector);
  vector->size--;
  *p = INVALID_VECTOR_ELEMENT;
  kissat_inc_usable (vectors);
}

unsigned *kissat_enlarge_vector (struct kissat *, vectors *, vector *);
void kissat_defrag_vectors (struct kissat *, vectors *, unsigned, vector *);
void kissat_remove_from_vector (struct kissat *, vectors *, vector *,
				unsigned);
void kissat_resize_vector (struct kissat *, vectors *, vector *, sector);
void kissat_release_vector (struct kissat *, vectors *, vector *);

static inline void
kissat_dec_usable (vectors * vectors)
{
  assert (vectors->usable > 0);
  vectors->usable--;
}

static inline void
kissat_push_vectors (struct kissat *solver,
		     vectors * vectors, vector * vector, unsigned e)
{
  unsigneds *stack = &vectors->stack;
  assert (e != INVALID_VECTOR_ELEMENT);
  if (!vector->size && !vector->offset)
    {
      if (EMPTY_STACK (*stack))
	PUSH_STACK (*stack, 0);
      if (FULL_STACK (*stack))
	{
	  unsigned *end = kissat_enlarge_vector (solver, vectors, vector);
	  assert (*end == INVALID_VECTOR_ELEMENT);
	  *end = e;
	  kissat_dec_usable (vectors);
	}
      else
	{
	  assert ((uint64_t) SIZE_STACK (*stack) < MAX_VECTORS);
	  vector->offset = SIZE_STACK (*stack);
	  assert (vector->offset);
	  *stack->end++ = e;
	}
    }
  else
    {
      unsigned *end = kissat_end_vector (vectors, vector);
      if (end == END_STACK (*stack))
	{
	  if (FULL_STACK (*stack))
	    {
	      end = kissat_enlarge_vector (solver, vectors, vector);
	      assert (*end == INVALID_VECTOR_ELEMENT);
	      *end = e;
	      kissat_dec_usable (vectors);
	    }
	  else
	    *stack->end++ = e;
	}
      else
	{
	  if (*end != INVALID_VECTOR_ELEMENT)
	    end = kissat_enlarge_vector (solver, vectors, vector);
	  assert (*end == INVALID_VECTOR_ELEMENT);
	  *end = e;
	  kissat_dec_usable (vectors);
	}
    }
  vector->size++;
  kissat_check_vectors (solver);
}

#define all_vector(E,V,VS) \
  unsigned E, * E_PTR = kissat_begin_vector (VS, &V), \
              * E_END = E_PTR + (V).size; \
  E_PTR != E_END && (E = *E_PTR, true); \
  E_PTR++

#endif
