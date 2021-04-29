#include "kitten.h"
#include "random.h"
#include "stack.h"

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------------*/
#ifdef STAND_ALONE_KITTEN
/*------------------------------------------------------------------------*/

// Replacement for 'kissat' allocators in the stand alone variant.

typedef signed char value;

static void
die (const char *fmt, ...)
{
  fputs ("kitten: error: ", stderr);
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

static inline void *
kitten_calloc (size_t n, size_t size)
{
  void *res = calloc (n, size);
  if (n && size && !res)
    die ("out of memory allocating '%zu * %zu' bytes", n, size);
  return res;
}

#define CALLOC(P,N) do { (P) = kitten_calloc (N, sizeof *(P)); } while (0)
#define DEALLOC(P,N) free (P)

#undef ENLARGE_STACK

#define ENLARGE_STACK(S) \
do { \
  assert (FULL_STACK (S)); \
  const size_t SIZE = SIZE_STACK (S); \
  const size_t OLD_CAPACITY = CAPACITY_STACK (S); \
  const size_t NEW_CAPACITY = OLD_CAPACITY ? 2*OLD_CAPACITY : 1; \
  const size_t BYTES = NEW_CAPACITY * sizeof *(S).begin; \
  (S).begin = realloc ((S).begin, BYTES); \
  if (!(S).begin) \
    die ("out of memory reallocating '%zu' bytes", BYTES); \
  (S).allocated = (S).begin + NEW_CAPACITY; \
  (S).end = (S).begin + SIZE; \
} while (0)

// Beside allocators above also use stand alone statistics counters.

#define INC(NAME) \
do { \
  statistics * statistics = &kitten->statistics; \
  assert (statistics->NAME < UINT64_MAX); \
  statistics->NAME++; \
} while (0)

#define ADD(NAME,DELTA) \
do { \
  statistics * statistics = &kitten->statistics; \
  assert (statistics->NAME <= UINT64_MAX - (DELTA)); \
  statistics->NAME += (DELTA); \
} while (0)

/*------------------------------------------------------------------------*/
#else // STAND_ALONE_KITTEN
/*------------------------------------------------------------------------*/

#include "allocate.h"		// Use 'kissat' allocator if embedded.
#include "error.h"		// Use 'kissat_fatal' if embedded.
#include "internal.h"		// Also use 'kissat' statistics if embedded.

/*------------------------------------------------------------------------*/
#endif // STAND_ALONE_KITTEN
/*------------------------------------------------------------------------*/

#define INVALID UINT_MAX
#define MAX_VARS ((1u<<31)-1)

#define CORE_FLAG (1u)
#define LEARNED_FLAG (2u)

// *INDENT-OFF*

typedef struct kar kar;
typedef struct kink kink;
typedef struct klause klause;
typedef STACK (unsigned) klauses;
typedef unsigneds katches;

// *INDENT-ON*

struct kar
{
  unsigned level;
  unsigned reason;
};

struct kink
{
  unsigned next;
  unsigned prev;
  uint64_t stamp;
};

struct klause
{
  unsigned aux;
  unsigned size;
  unsigned flags;
  unsigned lits[1];
};

#ifdef STAND_ALONE_KITTEN

typedef struct statistics statistics;

struct statistics
{
  uint64_t learned;
  uint64_t original;
  uint64_t kitten_sat;
  uint64_t kitten_solved;
  uint64_t kitten_conflicts;
  uint64_t kitten_decisions;
  uint64_t kitten_propagations;
  uint64_t kitten_unsat;
};

#endif

struct kitten
{
#ifndef STAND_ALONE_KITTEN
  struct kissat *kissat;
#define solver (kitten->kissat)
#endif

  // First zero initialized field in 'clear_kitten' is 'status'.
  //
  int status;

#if defined(STAND_ALONE_KITTEN) && defined(LOGGING)
  bool logging;
#endif
  bool antecedents;
  bool learned;

  unsigned level;
  unsigned propagated;
  unsigned unassigned;
  unsigned inconsistent;

  uint64_t generator;

  size_t lits;
  size_t end_original_ref;

  struct
  {
    unsigned first, last;
    uint64_t stamp;
    unsigned search;
  } queue;

  // The 'size' field below is the first not zero reinitialized field
  // by 'memset' in 'clear_kitten' (after 'kissat').

  size_t size;

  kar *vars;
  kink *links;
  value *marks;
  value *values;
  unsigned char *phases;
  katches *watches;

  unsigneds analyzed;
  unsigneds klause;
  unsigneds klauses;
  unsigneds resolved;
  unsigneds trail;
  unsigneds units;

#ifdef STAND_ALONE_KITTEN
  statistics statistics;
#endif
};

/*------------------------------------------------------------------------*/

static inline bool
is_core_klause (klause * c)
{
  return c->flags & CORE_FLAG;
}

static inline bool
is_learned_klause (klause * c)
{
  return c->flags & LEARNED_FLAG;
}

static inline void
set_core_klause (klause * c)
{
  c->flags |= CORE_FLAG;
}

static inline void
unset_core_klause (klause * c)
{
  c->flags &= ~CORE_FLAG;
}

static inline klause *
dereference_klause (kitten * kitten, unsigned ref)
{
  unsigned *res = BEGIN_STACK (kitten->klauses) + ref;
  assert (res < END_STACK (kitten->klauses));
  return (klause *) res;
}

/*------------------------------------------------------------------------*/

#define KATCHES(KIT) \
  (kitten->watches[assert ((KIT) < kitten->lits), (KIT)])

#define all_original_klauses(C) \
  klause * C = begin_klauses (kitten), \
    * end_ ## C = end_original_klauses (kitten); \
  (C) != end_ ## C; \
  (C) = next_klause (kitten, C)

#define all_learned_klauses(C) \
  klause * C = begin_learned_klauses (kitten), \
    * end_ ## C = end_klauses (kitten); \
  (C) != end_ ## C; \
  (C) = next_klause (kitten, C)

#define all_kits(KIT) \
  size_t KIT = 0, KIT_ ## END = kitten->lits; KIT != KIT_ ## END; KIT++

#define BEGIN_KLAUSE(C) (C)->lits

#define END_KLAUSE(C) (BEGIN_KLAUSE (C) + (C)->size)

#define all_literals_in_klause(KIT,C) \
  unsigned KIT, * KIT ## _PTR = BEGIN_KLAUSE (C), \
                * KIT ## _END = END_KLAUSE (C); \
  KIT ## _PTR != KIT ## _END && ((KIT = *KIT ## _PTR), true); \
  ++KIT ## _PTR

#define all_antecedents(REF,C) \
  unsigned REF, * REF ## _PTR = antecedents (C), \
	      * REF ## _END = REF ## _PTR + (C)->aux; \
  REF ## _PTR != REF ## _END && ((REF = *REF ## _PTR), true); \
  ++REF ## _PTR

#ifdef LOGGING

#ifdef STAND_ALONE_KITTEN
#define logging (kitten->logging)
#else
#define logging GET_OPTION(log)
#endif

static void
log_basic (kitten * kitten, const char *fmt, ...)
{
  assert (logging);
  printf ("c KITTEN %u ", kitten->level);
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

static void
log_reference (kitten * kitten, unsigned ref, const char *fmt, ...)
{
  klause *c = dereference_klause (kitten, ref);
  assert (logging);
  printf ("c KITTEN %u ", kitten->level);
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  if (is_learned_klause (c))
    {
      fputs (" learned", stdout);
      if (c->aux)
	printf ("[%u]", c->aux);
    }
  else
    printf (" original[%u]", c->aux);
  printf (" size %u clause[%u]", c->size, ref);
  value *values = kitten->values;
  kar *vars = kitten->vars;
  for (all_literals_in_klause (lit, c))
    {
      printf (" %u", lit);
      const value value = values[lit];
      if (value)
	printf ("@%u=%d", vars[lit / 2].level, (int) value);
    }
  fputc ('\n', stdout);
  fflush (stdout);
}

#define LOG(...) \
do { \
  if (logging) \
    log_basic (kitten, __VA_ARGS__); \
} while (0)

#define ROG(...) \
do { \
  if (logging) \
    log_reference (kitten, __VA_ARGS__); \
} while (0)

#else

#define LOG(...) do { } while (0)
#define ROG(...) do { } while (0)

#endif

static void
check_queue (kitten * kitten)
{
#ifdef CHECK_KITTEN
  const unsigned vars = kitten->lits / 2;
  unsigned found = 0, prev = INVALID;
  kink *links = kitten->links;
  uint64_t stamp = 0;
  for (unsigned idx = kitten->queue.first, next; idx != INVALID; idx = next)
    {
      kink *link = links + idx;
      assert (link->prev == prev);
      assert (!found || stamp < link->stamp);
      assert (link->stamp < kitten->queue.stamp);
      stamp = link->stamp;
      next = link->next;
      prev = idx;
      found++;
    }
  assert (found == vars);
  unsigned next = INVALID;
  found = 0;
  for (unsigned idx = kitten->queue.last, prev; idx != INVALID; idx = prev)
    {
      kink *link = links + idx;
      assert (link->next == next);
      prev = link->prev;
      next = idx;
      found++;
    }
  assert (found == vars);
  value *values = kitten->values;
  bool first = true;
  for (unsigned idx = kitten->queue.search, next; idx != INVALID; idx = next)
    {
      kink *link = links + idx;
      next = link->next;
      const unsigned lit = 2 * idx;
      assert (first || values[lit]);
      first = false;
    }
#else
  (void) kitten;
#endif
}

static void
update_search (kitten * kitten, unsigned idx)
{
  if (kitten->queue.search == idx)
    return;
  kitten->queue.search = idx;
  LOG ("search updated to %u stamped %" PRIu64,
       idx, kitten->links[idx].stamp);
}

static void
enqueue (kitten * kitten, unsigned idx)
{
  LOG ("enqueue %u", idx);
  kink *links = kitten->links;
  kink *l = links + idx;
  const unsigned last = kitten->queue.last;
  if (last == INVALID)
    kitten->queue.first = idx;
  else
    links[last].next = idx;
  l->prev = last;
  l->next = INVALID;
  kitten->queue.last = idx;
  l->stamp = kitten->queue.stamp++;
  LOG ("stamp %" PRIu64, l->stamp);
}

static void
dequeue (kitten * kitten, unsigned idx)
{
  LOG ("dequeue %u", idx);
  kink *links = kitten->links;
  kink *l = links + idx;
  const unsigned prev = l->prev;
  const unsigned next = l->next;
  if (prev == INVALID)
    kitten->queue.first = next;
  else
    links[prev].next = next;
  if (next == INVALID)
    kitten->queue.last = prev;
  else
    links[next].prev = prev;
}

static void
init_queue (kitten * kitten, size_t old_vars, size_t new_vars)
{
  for (size_t idx = old_vars; idx < new_vars; idx++)
    {
      assert (!kitten->values[2 * idx]);
      assert (kitten->unassigned < UINT_MAX);
      kitten->unassigned++;
      enqueue (kitten, idx);
    }
  LOG ("initialized decision queue from %zu to %zu", old_vars, new_vars);
  update_search (kitten, kitten->queue.last);
  check_queue (kitten);
}

static void
initialize_kitten (kitten * kitten)
{
  kitten->queue.first = INVALID;
  kitten->queue.last = INVALID;
  kitten->inconsistent = INVALID;
  kitten->queue.search = INVALID;
}

static void
clear_kitten (kitten * kitten)
{
  size_t bytes = (char *) &kitten->size - (char *) &kitten->status;
  memset (&kitten->status, 0, bytes);
#ifdef STAND_ALONE_KITTEN
  memset (&kitten->statistics, 0, sizeof (statistics));
#endif
  initialize_kitten (kitten);
}

#define RESIZE1(P) \
do { \
  void * OLD_PTR = (P); \
  CALLOC ((P), new_size/2); \
  const size_t BYTES = old_vars * sizeof *(P); \
  memcpy ((P), OLD_PTR, BYTES); \
  void * NEW_PTR = (P); \
  (P) = OLD_PTR; \
  DEALLOC ((P), old_size/2); \
  (P) = NEW_PTR; \
} while (0)

#define RESIZE2(P) \
do { \
  void * OLD_PTR = (P); \
  CALLOC ((P), new_size); \
  const size_t BYTES = old_lits * sizeof *(P); \
  memcpy ((P), OLD_PTR, BYTES); \
  void * NEW_PTR = (P); \
  (P) = OLD_PTR; \
  DEALLOC ((P), old_size); \
  (P) = NEW_PTR; \
} while (0)

static void
enlarge_kitten (kitten * kitten, size_t lit)
{
  const size_t new_lits = (lit | 1) + 1;
  const size_t old_lits = kitten->lits;
  assert (old_lits < new_lits);
  assert ((lit ^ 1) < new_lits);
  assert (lit < new_lits);
  const size_t old_size = kitten->size;
  const unsigned new_vars = new_lits / 2;
  const unsigned old_vars = old_lits / 2;
  if (old_size < new_lits)
    {
      size_t new_size = old_size ? 2 * old_size : 2;
      while (new_size <= lit)
	new_size *= 2;
      LOG ("resizing to %zu literals from %zu (requested %zu)",
	   new_size, old_size, new_lits);

      RESIZE1 (kitten->marks);
      RESIZE1 (kitten->phases);
      RESIZE2 (kitten->values);
      RESIZE1 (kitten->vars);
      RESIZE1 (kitten->links);
      RESIZE2 (kitten->watches);

      kitten->size = new_size;
    }
  kitten->lits = new_lits;
  init_queue (kitten, old_vars, new_vars);
  LOG ("enlarged to %zu literals", new_lits);
  return;
}

static const char *
status_to_string (int status)
{
  if (status == 10)
    return "formula satisfied";
  if (status == 20)
    return "formula inconsistent";
  if (status == 21)
    return "formula inconsistent and core computed";
  return "unsolved";
}

static void
invalid_api_usage (const char *fun, const char *fmt, ...)
{
  fprintf (stderr, "kitten: fatal error: invalid API usage in '%s': ", fun);
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
  abort ();
}

#define INVALID_API_USAGE(...) \
  invalid_api_usage (__func__, __VA_ARGS__)

#define REQUIRE_INITIALIZED() \
do { \
  if (!kitten) \
    INVALID_API_USAGE ("solver argument zero"); \
} while (0)

#define REQUIRE_STATUS(EXPECTED) \
do { \
  REQUIRE_INITIALIZED(); \
  if (kitten->status != (EXPECTED)) \
    INVALID_API_USAGE ("invalid status '%s' (expected '%s')", \
	                status_to_string (kitten->status), \
	                status_to_string (EXPECTED)); \
} while (0)

#define UPDATE_STATUS(STATUS) \
do { \
  assert (kitten->status != (STATUS)); \
  LOG ("updating status from '%s' to '%s'", \
       status_to_string (kitten->status), status_to_string (STATUS)); \
  kitten->status = (STATUS); \
} while (0)

#ifdef STAND_ALONE_KITTEN

kitten *
kitten_init (void)
{
  kitten *kitten;
  CALLOC (kitten, 1);
  initialize_kitten (kitten);
  return kitten;
}

#else

kitten *
kitten_embedded (struct kissat *kissat)
{
  if (!kissat)
    INVALID_API_USAGE ("'kissat' argument zero");

  kitten *kitten;
  struct kitten dummy;
  dummy.kissat = kissat;
  kitten = &dummy;
  CALLOC (kitten, 1);
  kitten->kissat = kissat;
  initialize_kitten (kitten);
  return kitten;
}

#endif

void
kitten_track_antecedents (kitten * kitten)
{
  REQUIRE_STATUS (0);

  if (kitten->learned)
    INVALID_API_USAGE ("can not start tracking antecedents after learning");

  LOG ("enabling antecedents tracking");
  kitten->antecedents = true;
}

static void
shuffle_unsigned_array (kitten * kitten, size_t size, unsigned *a)
{
  for (size_t i = 0; i < size; i++)
    {
      const size_t j = kissat_pick_random (&kitten->generator, 0, i);
      if (j == i)
	continue;
      const unsigned first = a[i];
      const unsigned second = a[j];
      a[i] = second;
      a[j] = first;
    }
}


static void
shuffle_unsigned_stack (kitten * kitten, unsigneds * stack)
{
  const size_t size = SIZE_STACK (*stack);
  unsigned *a = BEGIN_STACK (*stack);
  shuffle_unsigned_array (kitten, size, a);
}

static void
shuffle_katches (kitten * kitten)
{
  LOG ("shuffling watch lists");
  for (size_t lit = 0; lit < kitten->lits; lit++)
    shuffle_unsigned_stack (kitten, &KATCHES (lit));
}

static void
shuffle_queue (kitten * kitten)
{
  LOG ("shuffling variable decision order");

  const unsigned vars = kitten->lits / 2;
  for (unsigned i = 0; i < vars; i++)
    {
      const unsigned idx = kissat_pick_random (&kitten->generator, 0, vars);
      dequeue (kitten, idx);
      enqueue (kitten, idx);
    }
  update_search (kitten, kitten->queue.last);
}

static void
shuffle_units (kitten * kitten)
{
  LOG ("shuffling units");
  shuffle_unsigned_stack (kitten, &kitten->units);
}

void
kitten_shuffle (kitten * kitten)
{
  REQUIRE_STATUS (0);
  shuffle_queue (kitten);
  shuffle_katches (kitten);
  shuffle_units (kitten);
}

static inline unsigned *
antecedents (klause * c)
{
  assert (is_learned_klause (c));
  return c->lits + c->size;
}

static inline void
watch_klause (kitten * kitten, unsigned lit, unsigned ref)
{
  ROG (ref, "watching %u in", lit);
  katches *watches = &KATCHES (lit);
  PUSH_STACK (*watches, ref);
}

static inline void
inconsistent (kitten * kitten, unsigned ref)
{
  assert (ref != INVALID);
  if (kitten->inconsistent != INVALID)
    return;
  ROG (ref, "inconsistent");
  kitten->inconsistent = ref;
}

static inline void
connect_new_klause (kitten * kitten, unsigned ref)
{
  ROG (ref, "new");

  klause *c = dereference_klause (kitten, ref);

  if (!c->size)
    inconsistent (kitten, ref);
  else if (c->size == 1)
    {
      ROG (ref, "watching unit");
      PUSH_STACK (kitten->units, ref);
    }
  else
    {
      watch_klause (kitten, c->lits[0], ref);
      watch_klause (kitten, c->lits[1], ref);
    }
}

static unsigned
new_reference (kitten * kitten)
{
  size_t ref = SIZE_STACK (kitten->klauses);
  if (ref >= INVALID)
    {
#ifdef STAND_ALONE_KITTEN
      die ("maximum number of literals exhausted");
#else
      kissat_fatal ("kitten: maximum number of literals exhausted");
#endif
    }
  const unsigned res = (unsigned) ref;
  assert (res != INVALID);
  return res;
}

static void
new_original_klause (kitten * kitten, unsigned id,
		     size_t size, const unsigned *lits)
{
  unsigned res = new_reference (kitten);
  unsigneds *klauses = &kitten->klauses;
  PUSH_STACK (*klauses, id);
  PUSH_STACK (*klauses, (unsigned) size);
  PUSH_STACK (*klauses, 0);
  for (size_t i = 0; i < size; i++)
    PUSH_STACK (*klauses, lits[i]);
  connect_new_klause (kitten, res);
  kitten->end_original_ref = SIZE_STACK (*klauses);
#ifdef STAND_ALONE_KITTEN
  kitten->statistics.original++;
#endif
}

void
kitten_clause (kitten * kitten, unsigned id,
	       size_t size, const unsigned *lits)
{
  REQUIRE_STATUS (0);
  const unsigned *const end = lits + size;
  value *marks = kitten->marks;
  for (const unsigned *p = lits; p != end; p++)
    {
      const unsigned lit = *p;
      if (lit >= kitten->lits)
	{
	  enlarge_kitten (kitten, lit);
	  marks = kitten->marks;
	}
      assert (lit < kitten->lits);
      const unsigned idx = *p / 2;
      if (marks[idx])
	INVALID_API_USAGE ("variable '%u' of literal '%u' occurs twice",
			   idx, lit);
      marks[idx] = true;
    }
  for (const unsigned *p = lits; p != end; p++)
    marks[*p / 2] = false;
  new_original_klause (kitten, id, size, lits);
}

unsigned
new_learned_klause (kitten * kitten)
{
  unsigned res = new_reference (kitten);
  unsigneds *klauses = &kitten->klauses;
  const size_t size = SIZE_STACK (kitten->klause);
  assert (size <= UINT_MAX);
  const size_t aux = kitten->antecedents ? SIZE_STACK (kitten->resolved) : 0;
  assert (aux <= UINT_MAX);
  PUSH_STACK (*klauses, (unsigned) aux);
  PUSH_STACK (*klauses, (unsigned) size);
  PUSH_STACK (*klauses, LEARNED_FLAG);
  for (all_stack (unsigned, lit, kitten->klause))
      PUSH_STACK (*klauses, lit);
  if (aux)
    for (all_stack (unsigned, ref, kitten->resolved))
        PUSH_STACK (*klauses, ref);
  connect_new_klause (kitten, res);
  kitten->learned = true;
#ifdef STAND_ALONE_KITTEN
  kitten->statistics.learned++;
#endif
  return res;
}

void
kitten_clear (kitten * kitten)
{
  LOG ("clear kitten of size %zu", kitten->size);
  assert (EMPTY_STACK (kitten->analyzed));
  assert (EMPTY_STACK (kitten->klause));
  assert (EMPTY_STACK (kitten->resolved));

  CLEAR_STACK (kitten->trail);
  CLEAR_STACK (kitten->units);
  CLEAR_STACK (kitten->klauses);

  for (all_kits (kit))
    CLEAR_STACK (KATCHES (kit));

  const size_t lits = kitten->size;
  const unsigned vars = lits / 2;

#ifndef NDEBUG
  for (unsigned i = 0; i < vars; i++)
    assert (!kitten->marks[i]);
#endif
  memset (kitten->phases, 0, vars);
  memset (kitten->values, 0, lits);
  memset (kitten->vars, 0, vars);

  clear_kitten (kitten);
}

void
kitten_release (kitten * kitten)
{
  RELEASE_STACK (kitten->analyzed);
  RELEASE_STACK (kitten->klause);
  RELEASE_STACK (kitten->klauses);
  RELEASE_STACK (kitten->resolved);
  RELEASE_STACK (kitten->trail);
  RELEASE_STACK (kitten->units);

  for (size_t lit = 0; lit < kitten->size; lit++)
    RELEASE_STACK (kitten->watches[lit]);

#ifndef STAND_ALONE_KITTEN
  const size_t lits = kitten->size;
  const unsigned vars = lits / 2;
#endif
  DEALLOC (kitten->marks, vars);
  DEALLOC (kitten->phases, vars);
  DEALLOC (kitten->values, lits);
  DEALLOC (kitten->vars, vars);
  DEALLOC (kitten->links, vars);
  DEALLOC (kitten->watches, lits);
#ifdef STAND_ALONE_KITTEN
  free (kitten);
#else
  kissat_free (kitten->kissat, kitten, sizeof *kitten);
#endif
}

static inline void
move_to_front (kitten * kitten, unsigned idx)
{
  if (idx == kitten->queue.last)
    return;
  LOG ("move to front variable %u");
  dequeue (kitten, idx);
  enqueue (kitten, idx);
  assert (kitten->values[2 * idx]);
}

static inline void
assign (kitten * kitten, unsigned lit, unsigned reason)
{
#ifdef LOGGING
  if (reason == INVALID)
    LOG ("assign %u decision", lit);
  else
    ROG (reason, "assign %u reason", lit);
#endif
  value *values = kitten->values;
  const unsigned not_lit = lit ^ 1;
  assert (!values[lit]);
  assert (!values[not_lit]);
  values[lit] = 1;
  values[not_lit] = -1;
  const unsigned idx = lit / 2;
  const unsigned sign = lit & 1;
  kitten->phases[idx] = sign;
  PUSH_STACK (kitten->trail, lit);
  kar *v = kitten->vars + idx;
  v->level = kitten->level;
  v->reason = reason;
  assert (kitten->unassigned);
  kitten->unassigned--;
}

static inline unsigned
propagate_literal (kitten * kitten, unsigned lit)
{
  LOG ("propagating %u", lit);
  value *values = kitten->values;
  assert (values[lit] > 0);
  const unsigned not_lit = lit ^ 1;
  katches *watches = kitten->watches + not_lit;
  unsigned conflict = INVALID;
  unsigned *q = BEGIN_STACK (*watches);
  const unsigned *const end_watches = END_STACK (*watches);
  unsigned const *p = q;
  while (conflict == INVALID && p != end_watches)
    {
      const unsigned ref = *q++ = *p++;
      klause *c = dereference_klause (kitten, ref);
      assert (c->size > 1);
      unsigned *lits = c->lits;
      const unsigned other = lits[0] ^ lits[1] ^ not_lit;
      lits[0] = other, lits[1] = not_lit;
      const value other_value = values[other];
      if (other_value > 0)
	continue;
      value replacement_value = -1;
      unsigned replacement = INVALID;
      const unsigned *const end_lits = lits + c->size;
      unsigned *r;
      for (r = lits + 2; r != end_lits; r++)
	{
	  replacement = *r;
	  replacement_value = values[replacement];
	  if (replacement_value >= 0)
	    break;
	}
      if (replacement_value >= 0)
	{
	  assert (replacement != INVALID);
	  ROG (ref, "unwatching %u in", lit);
	  lits[1] = replacement;
	  *r = not_lit;
	  watch_klause (kitten, replacement, ref);
	  q--;
	}
      else if (other_value < 0)
	{
	  ROG (ref, "conflict");
	  INC (kitten_conflicts);
	  conflict = ref;
	}
      else
	{
	  assert (!other_value);
	  assign (kitten, other, ref);
	}
    }
  while (p != end_watches)
    *q++ = *p++;
  SET_END_OF_STACK (*watches, q);
  return conflict;
}

static inline unsigned
propagate (kitten * kitten)
{
  assert (kitten->inconsistent == INVALID);
  unsigned propagated = 0;
  unsigned conflict = INVALID;
  while (conflict == INVALID &&
	 kitten->propagated < SIZE_STACK (kitten->trail))
    {
      const unsigned lit = PEEK_STACK (kitten->trail, kitten->propagated);
      conflict = propagate_literal (kitten, lit);
      kitten->propagated++;
      propagated++;
    }
  ADD (kitten_propagations, propagated);
  return conflict;
}

static void
bump (kitten * kitten)
{
  value *marks = kitten->marks;
  for (all_stack (unsigned, idx, kitten->analyzed))
    {
      marks[idx] = 0;
      move_to_front (kitten, idx);
    }
  check_queue (kitten);
}

static inline void
unassign (kitten * kitten, value * values, unsigned lit)
{
  const unsigned not_lit = lit ^ 1;
  assert (values[lit] > 0);
  assert (values[not_lit] < 0);
  const unsigned idx = lit / 2;
#ifdef LOGGING
  kar *var = kitten->vars + idx;
  kitten->level = var->level;
  LOG ("unassign %u", lit);
#endif
  values[lit] = values[not_lit] = 0;
  assert (kitten->unassigned < kitten->lits / 2);
  kitten->unassigned++;
  kink *links = kitten->links;
  kink *link = links + idx;
  if (link->stamp > links[kitten->queue.search].stamp)
    update_search (kitten, idx);
}

static void
backtrack (kitten * kitten, unsigned jump)
{
  check_queue (kitten);
  assert (jump < kitten->level);
  LOG ("back%s to level %u",
       (kitten->level == jump + 1 ? "tracking" : "jumping"), jump);
  kar *vars = kitten->vars;
  value *values = kitten->values;
  while (!EMPTY_STACK (kitten->trail))
    {
      const unsigned lit = TOP_STACK (kitten->trail);
      const unsigned idx = lit / 2;
      const unsigned level = vars[idx].level;
      if (level == jump)
	break;
      (void) POP_STACK (kitten->trail);
      unassign (kitten, values, lit);
    }
  kitten->propagated = SIZE_STACK (kitten->trail);
  kitten->level = jump;
  check_queue (kitten);
}

static void
analyze (kitten * kitten, unsigned conflict_ref)
{
  assert (kitten->level);
  assert (kitten->inconsistent == INVALID);
  assert (EMPTY_STACK (kitten->analyzed));
  assert (EMPTY_STACK (kitten->resolved));
  assert (EMPTY_STACK (kitten->klause));
  PUSH_STACK (kitten->klause, INVALID);
  unsigned reason_ref = conflict_ref;
  value *marks = kitten->marks;
  const kar *const vars = kitten->vars;
  const unsigned level = kitten->level;
  unsigned const *p = END_STACK (kitten->trail);
  unsigned open = 0, jump = 0, size = 0, uip;
  for (;;)
    {
      assert (reason_ref != INVALID);
      klause *reason = dereference_klause (kitten, reason_ref);
      assert (reason);
      ROG (reason_ref, "analyzing");
      PUSH_STACK (kitten->resolved, reason_ref);
      for (all_literals_in_klause (lit, reason))
	{
	  const unsigned idx = lit / 2;
	  if (marks[idx])
	    continue;
	  assert (kitten->values[lit] < 0);
	  LOG ("analyzed %u", lit);
	  marks[idx] = true;
	  PUSH_STACK (kitten->analyzed, idx);
	  const kar *const v = vars + idx;
	  const unsigned tmp = v->level;
	  if (tmp < level)
	    {
	      if (tmp > jump)
		{
		  jump = tmp;
		  if (size > 1)
		    {
		      const unsigned other = PEEK_STACK (kitten->klause, 1);
		      POKE_STACK (kitten->klause, 1, lit);
		      lit = other;
		    }
		}
	      PUSH_STACK (kitten->klause, lit);
	      size++;
	    }
	  else
	    open++;
	}
      unsigned idx;
      do
	{
	  assert (BEGIN_STACK (kitten->trail) < p);
	  uip = *--p;
	}
      while (!marks[idx = uip / 2]);
      assert (open);
      if (!--open)
	break;
      reason_ref = vars[idx].reason;
    }
  const unsigned not_uip = uip ^ 1;
  LOG ("first UIP %u jump level %u size %u", not_uip, jump, size);
  POKE_STACK (kitten->klause, 0, not_uip);
  bump (kitten);
  CLEAR_STACK (kitten->analyzed);
  const unsigned learned_ref = new_learned_klause (kitten);
  CLEAR_STACK (kitten->resolved);
  CLEAR_STACK (kitten->klause);
  backtrack (kitten, jump);
  assign (kitten, not_uip, learned_ref);
}

static void
decide (kitten * kitten)
{
  assert (kitten->unassigned);
  unsigned idx = kitten->queue.search;
  const value *const values = kitten->values;
  const kink *const links = kitten->links;
  for (;;)
    {
      assert (idx != INVALID);
      if (!values[2 * idx])
	break;
      idx = links[idx].prev;
    }
  update_search (kitten, idx);
  INC (kitten_decisions);
  kitten->level++;
  const unsigned phase = kitten->phases[idx];
  const unsigned lit = 2 * idx + phase;
  LOG ("decision %u variable %u phase %u", lit, idx, phase);
  assign (kitten, lit, 0);
}

static int
propagate_units (kitten * kitten)
{
  if (kitten->inconsistent != INVALID)
    return 20;

  if (EMPTY_STACK (kitten->units))
    {
      LOG ("no root level unit clauses");
      return 0;
    }

  LOG ("propagating %zu root level unit clauses", SIZE_STACK (kitten->units));

  const value *const values = kitten->values;
  for (all_stack (unsigned, ref, kitten->units))
    {
      assert (ref != INVALID);
      klause *c = dereference_klause (kitten, ref);
      assert (c->size == 1);
      ROG (ref, "propagating unit");
      const unsigned unit = c->lits[0];
      const value value = values[unit];
      if (value > 0)
	continue;
      if (value < 0)
	{
	  inconsistent (kitten, ref);
	  return 20;
	}
      assign (kitten, unit, ref);
      const unsigned conflict = propagate (kitten);
      if (conflict == INVALID)
	continue;
      inconsistent (kitten, conflict);
      return 20;
    }
  return 0;
}

int
kitten_solve (kitten * kitten)
{
  REQUIRE_STATUS (0);

  INC (kitten_solved);

  unsigned wait = 100;

  int res = propagate_units (kitten);
  while (!res)
    {
      const unsigned conflict = propagate (kitten);
      if (conflict != INVALID)
	{
	  if (kitten->level)
	    analyze (kitten, conflict);
	  else
	    {
	      inconsistent (kitten, conflict);
	      res = 20;
	    }
	  if (wait)
	    wait--;
	}
      else if (!kitten->unassigned)
	res = 10;
      else if (kitten->level && !wait)
	{
	  backtrack (kitten, 0);
	  wait = 100;
	}
      else
	decide (kitten);
    }

  UPDATE_STATUS (res);

  if (res == 10)
    INC (kitten_sat);
  if (res == 20)
    INC (kitten_unsat);

  return res;
}

unsigned
kitten_compute_clausal_core (kitten * kitten, uint64_t * learned_ptr)
{
  REQUIRE_STATUS (20);

  if (!kitten->antecedents)
    INVALID_API_USAGE ("antecedents not tracked");

  LOG ("computing clausal core");

  unsigneds *resolved = &kitten->resolved;
  unsigneds *analyzed = &kitten->analyzed;

  const kar *const vars = kitten->vars;
  value *marks = kitten->marks;

  assert (EMPTY_STACK (*resolved));
  assert (EMPTY_STACK (*analyzed));

  unsigned original = 0;
  uint64_t learned = 0;

  bool learn_additional_empty_clause = false;
  unsigned reason_ref = kitten->inconsistent;
  assert (reason_ref != INVALID);

  for (;;)
    {
      ROG (reason_ref, "analyzing core");
      assert (reason_ref != INVALID);
      klause *reason = dereference_klause (kitten, reason_ref);
      assert (!is_core_klause (reason));
      set_core_klause (reason);
      if (is_learned_klause (reason))
	learned++;
      else
	original++;

      PUSH_STACK (*resolved, reason_ref);

      for (all_literals_in_klause (lit, reason))
	{
	  const unsigned idx = lit / 2;
	  if (marks[idx])
	    continue;
	  assert (kitten->values[lit] < 0);
	  marks[idx] = 1;
	  PUSH_STACK (*analyzed, idx);
	  learn_additional_empty_clause = true;
	}

      if (EMPTY_STACK (*analyzed))
	break;

      const unsigned idx = POP_STACK (*analyzed);
      assert (marks[idx]);
      const kar *const v = vars + idx;
      assert (!v->level);
      reason_ref = v->reason;
    }
  memset (marks, 0, kitten->lits / 2);

  if (learn_additional_empty_clause)
    {
      unsigned new_inconsistent = new_learned_klause (kitten);
      ROG (new_inconsistent, "updated inconsistent");
      kitten->inconsistent = new_inconsistent;
      klause *c = dereference_klause (kitten, new_inconsistent);
      set_core_klause (c);
      learned++;
    }

  while (!EMPTY_STACK (*resolved))
    {
      const unsigned c_ref = POP_STACK (*resolved);
      ROG (c_ref, "analyzing core");
      klause *c = dereference_klause (kitten, c_ref);
      assert (is_core_klause (c));
      if (!is_learned_klause (c))
	continue;
      for (all_antecedents (d_ref, c))
	{
	  klause *d = dereference_klause (kitten, d_ref);
	  if (is_core_klause (d))
	    continue;
	  set_core_klause (d);
	  if (is_learned_klause (d))
	    {
	      learned++;
	      PUSH_STACK (*resolved, d_ref);
	    }
	  else
	    original++;
	}
    }

  if (learned_ptr)
    *learned_ptr = learned;

  LOG ("clausal core of %u original clauses", original);
  LOG ("clausal core of %" PRIu64 " learned clauses", learned);
#ifdef STAND_ALONE_KITTEN
  kitten->statistics.original = original;
  kitten->statistics.learned = 0;
#endif

  UPDATE_STATUS (21);

  return original;
}

static klause *
begin_klauses (kitten * kitten)
{
  return (klause *) BEGIN_STACK (kitten->klauses);
}

static klause *
end_original_klauses (kitten * kitten)
{
  return (klause *) (BEGIN_STACK (kitten->klauses) +
		     kitten->end_original_ref);
}

static klause *
begin_learned_klauses (kitten * kitten)
{
  return end_original_klauses (kitten);
}

static klause *
end_klauses (kitten * kitten)
{
  return (klause *) END_STACK (kitten->klauses);
}

static klause *
next_klause (kitten * kitten, klause * c)
{
  assert (begin_klauses (kitten) <= c);
  assert (c < end_klauses (kitten));
  unsigned *res = c->lits + c->size;
  if (kitten->antecedents && is_learned_klause (c))
    res += c->aux;
  return (klause *) res;
}

void
kitten_traverse_clausal_core (kitten * kitten, void *state,
			      void (*traverse) (void *, unsigned))
{
  REQUIRE_STATUS (21);

  LOG ("traversing core of original clauses");

  unsigned traversed = 0;

  for (all_original_klauses (c))
    {
      assert (!is_learned_klause (c));
      if (is_learned_klause (c))
	continue;
      if (!is_core_klause (c))
	continue;
      traverse (state, c->aux);
      traversed++;
    }

  LOG ("traversed %u original core clauses", traversed);
  (void) traversed;

  assert (kitten->status == 21);
}

void
kitten_traverse_core_lemmas (kitten * kitten, void *state,
			     void (*traverse) (void *,
					       size_t, const unsigned *))
{
  REQUIRE_STATUS (21);

  LOG ("traversing learned core lemmas");

  unsigned traversed = 0;

  for (all_learned_klauses (c))
    {
      assert (is_learned_klause (c));
      if (!is_learned_klause (c))
	continue;
      if (!is_core_klause (c))
	continue;
      traverse (state, c->size, c->lits);
      traversed++;
    }

  LOG ("traversed %u learned core lemmas", traversed);
  (void) traversed;

  assert (kitten->status == 21);
}

void
kitten_shrink_to_clausal_core (kitten * kitten)
{
  REQUIRE_STATUS (21);

  LOG ("shrinking formula to core of original clauses");

  CLEAR_STACK (kitten->trail);

  kitten->unassigned = kitten->lits / 2;
  kitten->propagated = 0;
  kitten->level = 0;

  update_search (kitten, kitten->queue.last);

  memset (kitten->values, 0, kitten->lits);

  for (all_kits (lit))
    CLEAR_STACK (KATCHES (lit));

  assert (kitten->inconsistent != INVALID);
  klause *inconsistent = dereference_klause (kitten, kitten->inconsistent);
  if (is_learned_klause (inconsistent) || inconsistent->size)
    {
      ROG (kitten->inconsistent, "resetting inconsistent");
      kitten->inconsistent = INVALID;
    }
  else
    ROG (kitten->inconsistent, "keeping inconsistent");

  CLEAR_STACK (kitten->units);

  klause *begin = begin_klauses (kitten), *q = begin;
  klause const *const end = end_original_klauses (kitten);
  unsigned original = 0;
  for (klause * c = begin, *next; c != end; c = next)
    {
      next = next_klause (kitten, c);
      assert (!is_learned_klause (c));
      if (is_learned_klause (c))
	continue;
      if (!is_core_klause (c))
	continue;
      unset_core_klause (c);
      const unsigned dst = (unsigned *) q - (unsigned *) begin;
      const unsigned size = c->size;
      if (!size)
	{
	  if (!kitten->inconsistent)
	    kitten->inconsistent = dst;
	}
      else if (size == 1)
	PUSH_STACK (kitten->units, dst);
      else
	{
	  watch_klause (kitten, c->lits[0], dst);
	  watch_klause (kitten, c->lits[1], dst);
	}
      if (c == q)
	q = next;
      else
	{
	  const size_t bytes = (char *) next - (char *) c;
	  memmove (q, c, bytes);
	  q = (klause *) ((char *) q + bytes);
	}
      original++;
    }
  SET_END_OF_STACK (kitten->klauses, (unsigned *) q);
  kitten->end_original_ref = SIZE_STACK (kitten->klauses);
  LOG ("end of original clauses at %zu", kitten->end_original_ref);
  LOG ("%u original clauses left", original);

  UPDATE_STATUS (0);
}

#ifdef STAND_ALONE_KITTEN

#include <ctype.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>

static double
process_time (void)
{
  struct rusage u;
  double res;
  if (getrusage (RUSAGE_SELF, &u))
    return 0;
  res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
  res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
  return res;
}

static double
maximum_resident_set_size (void)
{
  struct rusage u;
  if (getrusage (RUSAGE_SELF, &u))
    return 0;
  const uint64_t bytes = ((uint64_t) u.ru_maxrss) << 10;
  return bytes / (double) (1 << 20);
}

#include "attribute.h"

static void
msg (const char *, ...)
ATTRIBUTE_FORMAT (1, 2);

     static void msg (const char *fmt, ...)
{
  fputs ("c ", stdout);
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

#undef logging

typedef struct parser parser;

struct parser
{
  const char *path;
  uint64_t lineno;
  FILE *file;
#ifdef LOGGING
  bool logging;
#endif
};

static void
parse_error (parser * parser, const char *msg)
{
  fprintf (stderr, "kitten: error: %s:%" PRIu64 ": parse error: %s\n",
	   parser->path, parser->lineno, msg);
  exit (1);
}

#define ERROR(...) parse_error (parser, __VA_ARGS__)

static inline int
next_char (parser * parser)
{
  int res = getc (parser->file);
  if (res == '\r')
    {
      res = getc (parser->file);
      if (res != '\n')
	ERROR ("expected new line after carriage return");
    }
  if (res == '\n')
    parser->lineno++;
  return res;
}

#define NEXT() next_char (parser)

#define FAILED INT_MIN

static int
parse_number (parser * parser, int *res_ptr)
{
  int ch = NEXT ();
  if (!isdigit (ch))
    return FAILED;
  int res = ch - '0';
  while (isdigit (ch = NEXT ()))
    {
      if (!res)
	return FAILED;
      if (INT_MAX / 10 < res)
	return FAILED;
      res *= 10;
      const int digit = ch - '0';
      if (INT_MAX - digit < res)
	return FAILED;
      res += digit;
    }
  *res_ptr = res;
  return ch;
}

static kitten *
parse (parser * parser, ints * originals, int *max_var)
{
  int last = '\n', ch;
  for (;;)
    {
      ch = NEXT ();
      if (ch == 'c')
	{
	  while ((ch = NEXT ()) != '\n')
	    if (ch == EOF)
	      ERROR ("unexpected end-of-file in comment");
	}
      else if (ch != ' ' && ch != '\t' && ch != '\n')
	break;
      last = ch;
    }
  if (ch != 'p' || last != '\n')
    ERROR ("expected 'c' or 'p' at start of line");
  bool valid = (NEXT () == ' ' && NEXT () == 'c' && NEXT () == 'n' &&
		NEXT () == 'f' && NEXT () == ' ');
  int vars = 0;
  int clauses = 0;
  if (valid && (parse_number (parser, &vars) != ' ' ||
		parse_number (parser, &clauses) != '\n'))
    valid = false;
  if (!valid)
    ERROR ("invalid header");
  msg ("found header 'p cnf %d %d'", vars, clauses);
  *max_var = vars;
  kitten *kitten = kitten_init ();
#ifdef LOGGING
  kitten->logging = parser->logging;
#define logging (kitten->logging)
#endif
  unsigneds clause;
  value *marks = kitten->marks;
  INIT_STACK (clause);
  int ilit = 0;
  int parsed = 0;
  bool tautological = false;
  uint64_t offset = 0;
  for (;;)
    {
      ch = NEXT ();
      if (ch == EOF)
	{
	  if (parsed < clauses)
	    ERROR ("clause missing");
	  if (ilit)
	    ERROR ("zero missing");
	  break;
	}
      if (ch == ' ' || ch == '\t' || ch == '\n')
	continue;
      if (ch == 'c')
	{
	  while ((ch = NEXT ()) != '\n')
	    if (ch == EOF)
	      ERROR ("unexpected end-of-file in comment");
	  continue;
	}
      int sign = 1;
      if (ch == '-')
	sign = -1;
      else
	ungetc (ch, parser->file);
      ch = parse_number (parser, &ilit);
      if (ch == FAILED)
	ERROR ("invalid literal");
      if (ch == EOF)
	ERROR ("unexpected end-of-file after literal");
      if (ch == 'c')
	{
	  while ((ch = NEXT ()) != '\n')
	    if (ch == EOF)
	      ERROR ("unexpected end-of-file in comment");
	}
      else if (ch != ' ' && ch != '\t' && ch != '\n')
	ERROR ("expected comment or white space after literal");
      if (ilit > vars)
	ERROR ("maximum variable exceeded");
      if (parsed == clauses)
	ERROR ("too many clauses");
      if (originals)
	PUSH_STACK (*originals, sign * ilit);
      if (!ilit)
	{
	  for (all_stack (unsigned, ulit, clause))
	      marks[ulit / 2] = 0;
	  if (tautological)
	    {
	      LOG ("skipping tautological clause");
	      tautological = false;
	    }
	  else if (offset > UINT_MAX)
	    ERROR ("too many original literals");
	  else
	    {
	      assert (SIZE_STACK (clause) <= UINT_MAX);
	      const unsigned size = SIZE_STACK (clause);
	      const unsigned *const lits = BEGIN_STACK (clause);
	      kitten_clause (kitten, offset, size, lits);
	    }
	  CLEAR_STACK (clause);
	  parsed++;

	  if (originals)
	    offset = SIZE_STACK (*originals);
	  else
	    offset = parsed;
	}
      else if (!tautological)
	{
	  const unsigned uidx = ilit - 1;
	  const unsigned ulit = 2 * uidx + (sign < 0);
	  if (ulit >= kitten->lits)
	    {
	      enlarge_kitten (kitten, ulit);
	      marks = kitten->marks;
	    }
	  const value mark = marks[uidx];
	  if (mark > 0)
	    LOG ("dropping repeated %u", ulit);
	  else if (mark < 0)
	    {
	      LOG ("clause contains both %u and %u", ulit ^ 1, ulit);
	      tautological = true;
	    }
	  else
	    {
	      marks[uidx] = sign;
	      PUSH_STACK (clause, ulit);
	    }
	}
    }
  RELEASE_STACK (clause);
  return kitten;
}

typedef struct line line;

struct line
{
  char buffer[80];
  size_t size;
};

static void
flush_line (line * line)
{
  if (!line->size)
    return;
  for (size_t i = 0; i < line->size; i++)
    fputc (line->buffer[i], stdout);
  fputc ('\n', stdout);
  line->size = 0;
}

static inline void
print_lit (line * line, int lit)
{
  char tmp[16];
  sprintf (tmp, " %d", lit);
  const size_t len = strlen (tmp);
  if (line->size + len > 78)
    flush_line (line);
  if (!line->size)
    {
      line->buffer[0] = 'v';
      line->size = 1;
    }
  strcpy (line->buffer + line->size, tmp);
  line->size += len;
}

signed char
kitten_value (kitten * kitten, unsigned lit)
{
  REQUIRE_STATUS (10);
  return (lit >= kitten->lits) ? -1 : kitten->values[lit];
}

static void
print_witness (kitten * kitten, int max_var)
{
  assert (max_var >= 0);
  line line = {.size = 0 };
  const size_t parsed_lits = 2 * (size_t) max_var;
  for (size_t ulit = 0; ulit < parsed_lits; ulit += 2)
    {
      const value sign = kitten_value (kitten, ulit);
      assert (sign);
      const int iidx = ulit / 2 + 1;
      const int ilit = sign * iidx;
      print_lit (&line, ilit);
    }
  print_lit (&line, 0);
  flush_line (&line);
}

static double
percent (double a, double b)
{
  return b ? 100 * a / b : 0;
}

typedef struct core_writer core_writer;

struct core_writer
{
  FILE *file;
  ints *originals;
#ifndef NDEBUG
  unsigned written;
#endif
};

static void
write_offset (void *ptr, unsigned offset)
{
  core_writer *writer = ptr;
#ifndef NDEBUG
  writer->written++;
#endif
  int const *p = &PEEK_STACK (*writer->originals, offset);
  FILE *file = writer->file;
  while (*p)
    fprintf (file, "%d ", *p++);
  fputs ("0\n", file);
}

static void
write_core (kitten * kitten, unsigned reduced, ints * originals, FILE * file)
{
  assert (originals);
  fprintf (file, "p cnf %zu %u\n", kitten->lits / 2, reduced);
  core_writer writer;
  writer.file = file;
  writer.originals = originals;
#ifndef NDEBUG
  writer.written = 0;
#endif
  kitten_traverse_clausal_core (kitten, &writer, write_offset);
  assert (writer.written == reduced);
}

#ifndef NDEBUG

typedef struct lemma_writer lemma_writer;

struct lemma_writer
{
  FILE *file;
  uint64_t written;
};

#endif

static void
write_lemma (void *ptr, size_t size, const unsigned *lits)
{
  const unsigned *const end = lits + size;
#ifdef NDEBUG
  FILE *file = ptr;
#else
  lemma_writer *writer = ptr;
  FILE *file = writer->file;
  writer->written++;
#endif
  for (const unsigned *p = lits; p != end; p++)
    {
      const unsigned ulit = *p;
      const unsigned idx = ulit / 2;
      const unsigned sign = ulit & 1;
      assert (idx + 1 <= (unsigned) INT_MAX);
      int ilit = idx + 1;
      if (sign)
	ilit = -ilit;
      fprintf (file, "%d ", ilit);
    }
  fputs ("0\n", file);
}

static void
write_lemmas (kitten * kitten, uint64_t reduced, FILE * file)
{
  void *state;
#ifdef NDEBUG
  state = file;
  (void) reduced;
#else
  lemma_writer writer;
  writer.file = file;
  writer.written = 0;
  state = &writer;
#endif
  kitten_traverse_core_lemmas (kitten, state, write_lemma);
  assert (writer.written == reduced);
}

static void
print_statistics (statistics statistics)
{
  msg ("conflicts:                 %20" PRIu64, statistics.kitten_conflicts);
  msg ("decisions:                 %20" PRIu64, statistics.kitten_decisions);
  msg ("propagations:              %20" PRIu64,
       statistics.kitten_propagations);
  msg ("maximum-resident-set-size: %23.2f MB", maximum_resident_set_size ());
  msg ("process-time:              %23.2f seconds", process_time ());
}

static volatile kitten *static_kitten;

#define SIGNALS \
SIGNAL(SIGABRT) \
SIGNAL(SIGBUS) \
SIGNAL(SIGINT) \
SIGNAL(SIGSEGV) \
SIGNAL(SIGTERM)

// *INDENT-OFF*

#define SIGNAL(SIG) \
static void (*SIG ## _handler)(int);
SIGNALS
#undef SIGNAL

// *INDENT-ON*

static void
reset_signals (void)
{
#define SIGNAL(SIG) \
  signal (SIG, SIG ## _handler);
  SIGNALS
#undef SIGNAL
    static_kitten = 0;
}

static const char *
signal_name (int sig)
{
#define SIGNAL(SIG) \
  if (sig == SIG) \
    return #SIG;
  SIGNALS
#undef SIGNAL
    return "SIGUNKNOWN";
}

static void
catch_signal (int sig)
{
  if (!static_kitten)
    return;
  statistics statistics = static_kitten->statistics;
  reset_signals ();
  msg ("caught signal %d (%s)", sig, signal_name (sig));
  print_statistics (statistics);
  raise (sig);
}

static void
init_signals (kitten * kitten)
{
  static_kitten = kitten;
#define SIGNAL(SIG) \
  SIG ##_handler = signal (SIG, catch_signal);
  SIGNALS
#undef SIGNAL
}

static bool
parse_arg (const char *arg, unsigned *res_ptr)
{
  unsigned res = 0;
  int ch;
  while ((ch = *arg++))
    {
      if (!isdigit (ch))
	return false;
      if (UINT_MAX / 10 < res)
	return false;
      res *= 10;
      const unsigned digit = ch - '0';
      if (UINT_MAX - digit < res)
	return false;
      res += digit;
    }
  *res_ptr = res;
  return true;
}

#undef logging

int
main (int argc, char **argv)
{
  const char *dimacs_path = 0;
  const char *output_path = 0;
  unsigned shrink = 0;
  bool witness = true;
  bool proof = false;
#ifdef LOGGING
  bool logging = false;
#endif
  for (int i = 1; i < argc; i++)
    {
      const char *arg = argv[i];
      if (!strcmp (arg, "-h"))
	printf ("usage: kitten [-h]"
#ifdef LOGGING
		" [-l]"
#endif
		" [-n] [-O[<n>]] [-p] [ <dimacs> [ <output> ] ]\n"), exit (0);
#ifdef LOGGING
      else if (!strcmp (arg, "-l"))
	logging = true;
#endif
      else if (!strcmp (arg, "-n"))
	witness = false;
      else if (!strcmp (arg, "-p"))
	proof = true;
      else if (arg[0] == '-' && arg[1] == 'O' && !arg[2])
	shrink = 1;
      else if (arg[0] == '-' && arg[1] == 'O' && parse_arg (arg + 2, &shrink))
	;
      else if (*arg == '-')
	die ("invalid option '%s' (try '-h')", arg);
      else if (output_path)
	die ("too many arguments (try '-h')");
      else if (dimacs_path)
	output_path = arg;
      else
	dimacs_path = arg;
    }
  if (proof && !output_path)
    die ("option '-p' without output file");
  FILE *dimacs_file;
  bool close_dimacs_file = true;
  if (!dimacs_path)
    close_dimacs_file = false, dimacs_file = stdin, dimacs_path = "<stdin>";
  else if (!(dimacs_file = fopen (dimacs_path, "r")))
    die ("can not open '%s' for reading", dimacs_path);
  msg ("Kitten SAT Solver");
  msg
    ("Copyright (c) 2020-2021, Armin Biere, Johannes Kepler University Linz");
  msg ("reading '%s'", dimacs_path);
  ints originals;
  INIT_STACK (originals);
  kitten *kitten;
  int max_var;
  {
    parser parser;
    parser.path = dimacs_path;
    parser.lineno = 1;
    parser.file = dimacs_file;
#ifdef LOGGING
    parser.logging = logging;
#endif
    kitten = parse (&parser,
		    ((output_path && !proof) ? &originals : 0), &max_var);
  }
  if (close_dimacs_file)
    fclose (dimacs_file);
  msg ("parsed DIMACS file in %.2f seconds", process_time ());
  init_signals (kitten);
  if (output_path)
    kitten_track_antecedents (kitten);
  int res = kitten_solve (kitten);
  if (res == 10)
    {
      printf ("s SATISFIABLE\n");
      fflush (stdout);
      if (witness)
	print_witness (kitten, max_var);
    }
  else if (res == 20)
    {
      printf ("s UNSATISFIABLE\n");
      fflush (stdout);
      if (output_path)
	{
	  const unsigned original = kitten->statistics.original;
	  const uint64_t learned = kitten->statistics.learned;
	  unsigned reduced_original, round = 0;
	  uint64_t reduced_learned;

	  for (;;)
	    {
	      msg ("computing clausal core of %" PRIu64 " clauses",
		   kitten->statistics.original + kitten->statistics.learned);

	      reduced_original =
		kitten_compute_clausal_core (kitten, &reduced_learned);

	      msg ("found %" PRIu64 " core lemmas %.0f%% "
		   "out of %" PRIu64 " learned clauses", reduced_learned,
		   percent (reduced_learned, learned), learned);

	      if (!shrink--)
		break;

	      msg ("shrinking round %u", ++round);

	      msg ("reduced to %u core clauses %.0f%% "
		   "out of %u original clauses", reduced_original,
		   percent (reduced_original, original), original);

	      kitten_shrink_to_clausal_core (kitten);
	      kitten_shuffle (kitten);

	      res = kitten_solve (kitten);
	      assert (res == 20);
	    }
	  FILE *output_file = fopen (output_path, "w");
	  if (!output_file)
	    die ("can not open '%s' for writing", output_path);
	  if (proof)
	    {
	      msg ("writing proof to '%s'", output_path);
	      write_lemmas (kitten, reduced_learned, output_file);
	      msg ("written %" PRIu64
		   " core lemmas %.0f%% of %" PRIu64 " learned clauses",
		   reduced_learned,
		   percent (reduced_learned, learned), learned);
	    }
	  else
	    {
	      msg ("writing original clausal core to '%s'", output_path);
	      write_core (kitten, reduced_original, &originals, output_file);
	      msg ("written %u core clauses %.0f%% of %u original clauses",
		   reduced_original,
		   percent (reduced_original, original), original);
	    }
	  fclose (output_file);
	}
    }
  RELEASE_STACK (originals);
  statistics statistics = kitten->statistics;
  reset_signals ();
  kitten_release (kitten);
  print_statistics (statistics);
  return res;
}

#endif
