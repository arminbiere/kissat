#ifndef NDEBUG

#include "check.h"
#include "error.h"
#include "internal.h"
#include "literal.h"
#include "logging.h"
#include "print.h"

#include <stdio.h>
#include <limits.h>

void
kissat_check_satisfying_assignment (kissat * solver)
{
  LOG ("checking satisfying assignment");
  const int *begin = BEGIN_STACK (solver->original);
  const int *end = END_STACK (solver->original), *q;
#ifdef LOGGING
  size_t count = 0;
#endif
  for (const int *p = begin; p != end; p = q + 1)
    {
      bool satisfied = false;
      int lit;
      for (q = p; (lit = *q); q++)
	if (!satisfied && kissat_value (solver, lit) == lit)
	  satisfied = true;
#ifdef LOGGING
      count++;
#endif
      if (satisfied)
	continue;
      kissat_fatal_message_start ();
      fputs ("unsatisfied clause:\n", stderr);
      for (q = p; (lit = *q); q++)
	fprintf (stderr, "%d ", lit);
      fputs ("0\n", stderr);
      fflush (stderr);
      kissat_abort ();
    }
  LOG ("assignment satisfies all %zu original clauses", count);
}

#include "allocate.h"
#include "inline.h"
#include "sort.h"

typedef struct hash hash;
typedef struct line line;

// *INDENT-OFF*

typedef STACK (line *) lines;

// *INDENT-ON*

struct line
{
  line *next;
  unsigned size;
  unsigned hash;
  unsigned lits[];
};

struct checker
{
  bool inconsistent;

  unsigned vars;
  unsigned size;

  unsigned lines;
  unsigned hashed;

  line **table;

  lines *watches;
  bool *marks;
  signed char *values;

  bool marked;
  unsigneds imported;

  unsigneds trail;
  unsigned propagated;

  unsigned nonces[32];

  uint64_t added;
  uint64_t checked;
  uint64_t collisions;
  uint64_t decisions;
  uint64_t propagations;
  uint64_t removed;
  uint64_t searches;
  uint64_t unchecked;
};

#define LOGIMPORTED3(...) \
  LOGUNSIGNEDS3 (SIZE_STACK (checker->imported), \
                 BEGIN_STACK (checker->imported), __VA_ARGS__)

#define LOGLINE3(...) \
  LOGUNSIGNEDS3 (line->size, line->lits, __VA_ARGS__)

#define MAX_NONCES \
  (sizeof checker->nonces / sizeof *checker->nonces)

static inline bool
less_unsigned (unsigned a, unsigned b)
{
  return a < b;
}

static void
sort_line (kissat * solver, checker * checker)
{
  SORT_STACK (unsigned, checker->imported, less_unsigned);
  LOGIMPORTED3 ("sorted checker");
}

static unsigned
hash_line (checker * checker)
{
  unsigned res = 0, pos = 0;
  for (all_stack (unsigned, lit, checker->imported))
    {
      res += checker->nonces[pos++] * lit;
      if (pos == MAX_NONCES)
	pos = 0;
    }
  return res;
}

static size_t
bytes_line (unsigned size)
{
  return sizeof (line) + size * sizeof (unsigned);
}

static void
init_nonces (kissat * solver, checker * checker)
{
  generator random = 42;
  for (unsigned i = 0; i < MAX_NONCES; i++)
    checker->nonces[i] = 1 | kissat_next_random32 (&random);
  LOG3 ("initialized %zu checker nonces", MAX_NONCES);
#ifndef LOGGING
  (void) solver;
#endif
}

void
kissat_init_checker (kissat * solver)
{
  LOG ("initializing internal proof checker");
  checker *checker = kissat_calloc (solver, 1, sizeof (struct checker));
  solver->checker = checker;
  init_nonces (solver, checker);
}

static void
release_hash (kissat * solver, checker * checker)
{
  for (unsigned h = 0; h < checker->hashed; h++)
    {
      for (line * line = checker->table[h], *next; line; line = next)
	{
	  next = line->next;
	  kissat_free (solver, line, bytes_line (line->size));
	}
    }
  kissat_dealloc (solver, checker->table, checker->hashed, sizeof (line *));
}

static void
release_watches (kissat * solver, checker * checker)
{
  const unsigned lits = 2 * checker->vars;
  for (unsigned i = 0; i < lits; i++)
    RELEASE_STACK (checker->watches[i]);
  kissat_dealloc (solver, checker->watches, 2 * checker->size,
		  sizeof (lines));
}

void
kissat_release_checker (kissat * solver)
{
  LOG ("releasing internal proof checker");
  checker *checker = solver->checker;
  release_hash (solver, checker);
  RELEASE_STACK (checker->imported);
  RELEASE_STACK (checker->trail);
  kissat_free (solver, checker->marks, 2 * checker->size * sizeof (bool));
  kissat_free (solver, checker->values, 2 * checker->size);
  release_watches (solver, checker);
  kissat_free (solver, checker, sizeof (struct checker));
}

#ifndef QUIET

#include <inttypes.h>

#define PERCENT_ADDED(NAME) \
  kissat_percent (checker->NAME, checker->added)

void
kissat_print_checker_statistics (kissat * solver, bool verbose)
{
  checker *checker = solver->checker;
  if (verbose)
    PRINT_STAT ("checker_added", checker->added, 100, "%", "");
  PRINT_STAT ("checker_checked", checker->checked,
	      PERCENT_ADDED (checked), "%", "added");
  if (verbose)
    PRINT_STAT ("checker_collisions", checker->collisions,
		kissat_percent (checker->collisions, checker->searches),
		"%", "per search");
  PRINT_STAT ("checker_decisions", checker->decisions,
	      kissat_average (checker->decisions, checker->checked),
	      "", "per check");
  PRINT_STAT ("checker_propagations", checker->propagations,
	      kissat_average (checker->propagations, checker->checked),
	      "", "per check");
  PRINT_STAT ("checker_removed", checker->removed,
	      PERCENT_ADDED (removed), "%", "added");
  PRINT_STAT ("checker_unchecked", checker->unchecked,
	      PERCENT_ADDED (unchecked), "%", "added");
}

#endif

#define MAX_VARS (1u<<29)
#define MAX_SIZE (1u<<30)

static unsigned
reduce_hash (unsigned hash, unsigned mod)
{
  if (mod < 2)
    return 0;
  assert (mod);
  unsigned res = hash;
  for (unsigned shift = 16, mask = 0xffff; res >= mod; mask >>= (shift >>= 1))
    res = (res >> shift) & mask;
  assert (res < mod);
  return res;
}

static void
resize_hash (kissat * solver, checker * checker)
{
  const unsigned old_hashed = checker->hashed;
  assert (old_hashed < MAX_SIZE);
  const unsigned new_hashed = old_hashed ? 2 * old_hashed : 1;
  line **table = kissat_calloc (solver, new_hashed, sizeof (line *));
  line **old_table = checker->table;
  for (unsigned i = 0; i < old_hashed; i++)
    {
      for (line * line = old_table[i], *next; line; line = next)
	{
	  next = line->next;
	  const unsigned reduced = reduce_hash (line->hash, new_hashed);
	  line->next = table[reduced];
	  table[reduced] = line;
	}
    }
  kissat_dealloc (solver, checker->table, old_hashed, sizeof (line *));
  checker->hashed = new_hashed;
  checker->table = table;
}

static line *
new_line (kissat * solver, checker * checker, unsigned size, unsigned hash)
{
  line *res = kissat_malloc (solver, bytes_line (size));
  res->next = 0;
  res->size = size;
  res->hash = hash;
  memcpy (res->lits, BEGIN_STACK (checker->imported),
	  size * sizeof *res->lits);
  return res;
}

#define CHECKER_LITS (2*(checker)->vars)
#define VALID_CHECKER_LIT(LIT) ((LIT) < CHECKER_LITS)

static line decision_line;
static line unit_line;

static void
checker_assign (kissat * solver, checker * checker, unsigned lit, line * line)
{
#ifdef LOGGING
  if (line == &decision_line)
    LOG3 ("checker assign %u (decision)", lit);
  else if (line == &unit_line)
    LOG3 ("checker assign %u (unit)", lit);
  else
    LOGLINE3 ("checker assign %u reason", lit);
#else
  (void) line;
#endif
  assert (VALID_CHECKER_LIT (lit));
  const unsigned not_lit = lit ^ 1;
  signed char *values = checker->values;
  assert (!values[lit]);
  assert (!values[not_lit]);
  values[lit] = 1;
  values[not_lit] = -1;
  PUSH_STACK (checker->trail, lit);
}

static lines *
checker_watches (checker * checker, unsigned lit)
{
  assert (VALID_CHECKER_LIT (lit));
  return checker->watches + lit;
}

static void
watch_checker_literal (kissat * solver, checker * checker, line * line,
		       unsigned lit)
{
  LOGLINE3 ("checker watches %u in", lit);
  lines *lines = checker_watches (checker, lit);
  PUSH_STACK (*lines, line);
}

static void
unwatch_checker_literal (kissat * solver, checker * checker, line * line,
			 unsigned lit)
{
  LOGLINE3 ("checker unwatches %u in", lit);
  lines *lines = checker_watches (checker, lit);
  REMOVE_STACK (struct line *, *lines, line);
#ifndef LOGGING
  (void) solver;
#endif
}

static void
unwatch_line (kissat * solver, checker * checker, line * line)
{
  assert (line->size > 1);
  const unsigned *lits = line->lits;
  unwatch_checker_literal (solver, checker, line, lits[0]);
  unwatch_checker_literal (solver, checker, line, lits[1]);
}

static bool
satisfied_or_trivial_imported (kissat * solver, checker * checker)
{
  const unsigned *lits = BEGIN_STACK (checker->imported);
  const unsigned *end_of_lits = END_STACK (checker->imported);
  const signed char *values = checker->values;
  bool *marks = checker->marks;
  const unsigned *p;
  bool res = false;
  for (p = lits; !res && p != end_of_lits; p++)
    {
      const unsigned lit = *p;
      if (marks[lit])
	continue;
      marks[lit] = true;
      const unsigned not_lit = lit ^ 1;
      if (marks[not_lit])
	{
	  LOGIMPORTED3 ("trivial by %u and %u imported checker",
			not_lit, lit);
	  res = true;
	}
      else if (values[lit] > 0)
	{
	  LOGIMPORTED3 ("satisfied by %u imported checker", lit);
	  res = true;
	}
    }
  for (const unsigned *q = lits; q != p; q++)
    marks[*q] = 0;
#ifndef LOGGING
  (void) solver;
#endif
  return res;
}

static void
mark_line (checker * checker)
{
  bool *marks = checker->marks;
  for (all_stack (unsigned, lit, checker->imported))
      marks[lit] = 1;
  checker->marked = true;
}

static void
unmark_line (checker * checker)
{
  bool *marks = checker->marks;
  for (all_stack (unsigned, lit, checker->imported))
      marks[lit] = 0;
  checker->marked = false;
}

static bool
simplify_imported (kissat * solver, checker * checker)
{
  if (checker->inconsistent)
    {
      LOG3 ("skipping addition since checker already inconsistent");
      return true;
    }
  unsigned non_false = 0;
#ifdef LOGGING
  unsigned num_false = 0;
#endif
  const unsigned *end_of_lits = END_STACK (checker->imported);
  unsigned *lits = BEGIN_STACK (checker->imported);
  const signed char *values = checker->values;
  bool *marks = checker->marks;
  bool res = false;
  unsigned *p;
  for (p = lits; !res && p != end_of_lits; p++)
    {
      const unsigned lit = *p;
      if (marks[lit])
	continue;
      marks[lit] = true;
      const unsigned not_lit = lit ^ 1;
      if (marks[not_lit])
	{
	  LOG3 ("simplified checker clause trivial (contains %u and %u)",
		not_lit, lit);
	  res = true;
	}
      else
	{
	  signed char lit_value = values[lit];
	  if (lit_value < 0)
	    {
#ifdef LOGGING
	      num_false++;
#endif
	    }
	  else if (lit_value > 0)
	    {
	      LOG3 ("simplified checker clause satisfied by %u", lit);
	      res = true;
	    }
	  else
	    {
	      if (!non_false)
		SWAP (unsigned, *p, lits[0]);
	      else if (non_false == 1)
		SWAP (unsigned, *p, lits[1]);
	      non_false++;
	    }
	}
    }
  for (const unsigned *q = lits; q != p; q++)
    marks[*q] = 0;
  if (!res)
    {
      if (!non_false)
	{
	  LOG3 ("simplified checker clause inconsistent");
	  checker->inconsistent = true;
	  res = true;
	}
      else if (non_false == 1)
	{
	  LOG3 ("simplified checker clause unit");
	  checker_assign (solver, checker, lits[0], &unit_line);
	  res = true;
	}
    }
  if (!res)
    {
      LOG3 ("non-trivial and non-satisfied imported checker clause "
	    "has %u false and %u non-false literals", num_false, non_false);
      LOGIMPORTED3 ("simplified checker");
    }
  return res;
}

static void
insert_imported (kissat * solver, checker * checker, unsigned hash)
{
  size_t size = SIZE_STACK (checker->imported);
  assert (size <= UINT_MAX);
  if (checker->lines == checker->hashed)
    resize_hash (solver, checker);
  line *line = new_line (solver, checker, size, hash);
  const unsigned reduced = reduce_hash (hash, checker->hashed);
  struct line **p = checker->table + reduced;
  line->next = *p;
  *p = line;
  LOGLINE3 ("inserted checker");
  const unsigned *lits = BEGIN_STACK (checker->imported);
  const signed char *values = checker->values;
  assert (!values[lits[0]]);
  assert (!values[lits[1]]);
  watch_checker_literal (solver, checker, line, lits[0]);
  watch_checker_literal (solver, checker, line, lits[1]);
  checker->lines++;
  checker->added++;
}

static void
insert_imported_if_not_simplified (kissat * solver, checker * checker)
{
  sort_line (solver, checker);
  const unsigned hash = hash_line (checker);
  if (!simplify_imported (solver, checker))
    insert_imported (solver, checker, hash);
}

static bool
match_line (checker * checker, unsigned size, unsigned hash, line * line)
{
  if (line->size != size)
    return false;
  if (line->hash != hash)
    return false;
  if (!checker->marked)
    mark_line (checker);
  const unsigned *lits = line->lits;
  const unsigned *end_of_lits = lits + line->size;
  const bool *marks = checker->marks;
  for (const unsigned *p = lits; p != end_of_lits; p++)
    if (!marks[*p])
      return false;
  return true;
}

static void
resize_checker (kissat * solver, checker * checker, unsigned new_vars)
{
  const unsigned vars = checker->vars;
  const unsigned size = checker->size;
  if (new_vars > size)
    {
      assert (new_vars <= MAX_SIZE);
      unsigned new_size = size ? 2 * size : 1;
      while (new_size < new_vars)
	new_size *= 2;
      assert (new_size <= MAX_SIZE);
      LOG3 ("resizing checker form %u to %u", size, new_size);
      const unsigned size2 = 2 * size;
      const unsigned new_size2 = 2 * new_size;
      checker->marks =
	kissat_realloc (solver, checker->marks, size2,
			new_size2 * sizeof (bool));
      checker->values =
	kissat_realloc (solver, checker->values, size2, new_size2);
      checker->watches =
	kissat_realloc (solver, checker->watches,
			size2 * sizeof *checker->watches,
			new_size2 * sizeof *checker->watches);
      checker->size = new_size;
    }
  const unsigned delta = new_vars - vars;
  if (delta == 1)
    LOG3 ("initializing one checker variable %u", vars);
  else
    LOG3 ("initializing %u checker variables from %u to %u",
	  delta, vars, new_vars - 1);
  const unsigned vars2 = 2 * vars;
  const unsigned new_vars2 = 2 * new_vars;
  const unsigned delta2 = 2 * delta;
  assert (delta2 == new_vars2 - vars2);
  memset (checker->watches + vars2, 0, delta2 * sizeof *checker->watches);
  memset (checker->marks + vars2, 0, delta2);
  memset (checker->values + vars2, 0, delta2);
  checker->vars = new_vars;
}

static inline unsigned
import_external_checker (kissat * solver, checker * checker, int elit)
{
  assert (elit);
  const unsigned var = ABS (elit) - 1;
  if (var >= checker->vars)
    resize_checker (solver, checker, var + 1);
  assert (var < checker->vars);
  return 2 * var + (elit < 0);
}

static inline unsigned
import_internal_checker (kissat * solver, checker * checker, unsigned ilit)
{
  const int elit = kissat_export_literal (solver, ilit);
  return import_external_checker (solver, checker, elit);
}

static inline int
export_checker (checker * checker, unsigned ilit)
{
  assert (ilit <= 2 * checker->vars);
  return (1 + (ilit >> 1)) * ((ilit & 1) ? -1 : 1);
}

static line *
find_line (kissat * solver, checker * checker, size_t size, bool remove)
{
  if (!checker->hashed)
    return 0;
  sort_line (solver, checker);
  checker->searches++;
  const unsigned hash = hash_line (checker);
  const unsigned reduced = reduce_hash (hash, checker->hashed);
  struct line **p, *line;
  for (p = checker->table + reduced;
       (line = *p) && !match_line (checker, size, hash, line);
       p = &line->next)
    checker->collisions++;
  if (checker->marked)
    unmark_line (checker);
  if (line && remove)
    *p = line->next;
  return line;
}

static void
remove_line (kissat * solver, checker * checker, size_t size)
{
  line *line = find_line (solver, checker, size, true);
  if (!line)
    {
      kissat_fatal_message_start ();
      fputs ("trying to remove non-existing clause:\n", stderr);
      for (all_stack (unsigned, lit, checker->imported))
	  fprintf (stderr, "%d ", export_checker (checker, lit));
      fputs ("0\n", stderr);
      fflush (stderr);
      kissat_abort ();
    }
  unwatch_line (solver, checker, line);
  LOGLINE3 ("removed checker");
  kissat_free (solver, line, bytes_line (size));
  assert (checker->lines > 0);
  checker->lines--;
  checker->removed++;
}

static void
import_external_literals (kissat * solver, checker * checker,
			  size_t size, int *elits)
{
  if (size > UINT_MAX)
    kissat_fatal ("can not check handle original clause of size %zu", size);
  CLEAR_STACK (checker->imported);
  for (size_t i = 0; i < size; i++)
    {
      const unsigned lit =
	import_external_checker (solver, checker, elits[i]);
      PUSH_STACK (checker->imported, lit);
    }
  LOGIMPORTED3 ("checker imported external");
}

static void
import_internal_literals (kissat * solver, checker * checker,
			  size_t size, const unsigned *ilits)
{
  assert (size <= UINT_MAX);
  CLEAR_STACK (checker->imported);
  for (size_t i = 0; i < size; i++)
    {
      const unsigned ilit = ilits[i];
      const unsigned lit = import_internal_checker (solver, checker, ilit);
      PUSH_STACK (checker->imported, lit);
    }
  LOGIMPORTED3 ("checker imported internal");
}


static void
import_clause (kissat * solver, checker * checker, clause * c)
{
  import_internal_literals (solver, checker, c->size, c->lits);
  LOGIMPORTED3 ("checker imported clause");
}

static void
import_binary (kissat * solver, checker * checker, unsigned a, unsigned b)
{
  CLEAR_STACK (checker->imported);
  const unsigned c = import_internal_checker (solver, checker, a);
  const unsigned d = import_internal_checker (solver, checker, b);
  PUSH_STACK (checker->imported, c);
  PUSH_STACK (checker->imported, d);
  LOGIMPORTED3 ("checker imported binary");
}

static void
import_internal_unit (kissat * solver, checker * checker, unsigned a)
{
  CLEAR_STACK (checker->imported);
  const unsigned b = import_internal_checker (solver, checker, a);
  PUSH_STACK (checker->imported, b);
  LOGIMPORTED3 ("checker imported unit");
}

static bool
checker_propagate (kissat * solver, checker * checker)
{
  unsigned propagated = checker->propagated;
  signed char *values = checker->values;
  bool res = true;
  while (res && propagated < SIZE_STACK (checker->trail))
    {
      const unsigned lit = PEEK_STACK (checker->trail, propagated);
      const unsigned not_lit = lit ^ 1;
      LOG3 ("checker propagate %u", lit);
      assert (values[lit] > 0);
      assert (values[not_lit] < 0);
      propagated++;
      lines *lines = checker_watches (checker, not_lit);
      line **begin_of_line = BEGIN_STACK (*lines), **q = begin_of_line;
      line *const *end_of_lines = END_STACK (*lines), *const *p = q;
      while (p != end_of_lines)
	{
	  line *line = *q++ = *p++;
	  if (!res)
	    continue;
	  unsigned *lits = line->lits;
	  const unsigned other = not_lit ^ lits[0] ^ lits[1];
	  const signed char other_value = values[other];
	  if (other_value > 0)
	    continue;
	  const unsigned *end_of_lits = lits + line->size;
	  unsigned replacement;
	  signed char replacement_value = -1;
	  unsigned *r;
	  for (r = lits + 2; r != end_of_lits; r++)
	    {
	      replacement = *r;
	      if (replacement == other)
		continue;
	      if (replacement == not_lit)
		continue;
	      replacement_value = values[replacement];
	      if (replacement_value >= 0)
		break;
	    }
	  if (replacement_value >= 0)
	    {
	      lits[0] = other;
	      lits[1] = replacement;
	      *r = not_lit;
	      LOGLINE3 ("checker unwatching %u in", not_lit);
	      watch_checker_literal (solver, checker, line, replacement);
	      q--;
	    }
	  else if (other_value < 0)
	    {
	      LOGLINE3 ("checker conflict");
	      res = false;
	    }
	  else
	    checker_assign (solver, checker, other, line);
	}
      SET_END_OF_STACK (*lines, q);
    }
  checker->propagations += propagated - checker->propagated;
  checker->propagated = propagated;
  return res;
}

static bool
line_redundant (kissat * solver, checker * checker, size_t size)
{
  if (!checker_propagate (solver, checker))
    {
      LOG3 ("root level checker unit propagations leads to conflict");
      LOG2 ("checker becomes inconsistent");
      checker->inconsistent = true;
      return true;
    }
  if (checker->inconsistent)
    {
      LOG3 ("skipping removal since checker already inconsistent");
      return true;
    }
  if (!size)
    kissat_fatal ("checker can not remove empty checker clause");
  if (size == 1)
    {
      const unsigned unit = PEEK_STACK (checker->imported, 0);
      const signed char value = checker->values[unit];
      if (value < 0 && !checker->inconsistent)
	kissat_fatal ("consistent checker can not remove falsified unit %d",
		      export_checker (checker, unit));
      if (!value)
	kissat_fatal ("checker can not remove unassigned unit %d",
		      export_checker (checker, unit));
      LOG3 ("checker skips removal of satisfied unit %u", unit);
      return true;
    }
  else if (satisfied_or_trivial_imported (solver, checker))
    {
      LOGIMPORTED3 ("satisfied imported checker");
      return true;
    }
  else
    return false;
}

static void
remove_line_if_not_redundant (kissat * solver, checker * checker)
{
  size_t size = SIZE_STACK (checker->imported);
  if (!line_redundant (solver, checker, size))
    remove_line (solver, checker, size);
}

static void
checker_backtrack (checker * checker, unsigned saved)
{
  unsigned *begin = BEGIN_STACK (checker->trail) + saved;
  unsigned *p = END_STACK (checker->trail);
  signed char *values = checker->values;
  while (p != begin)
    {
      const unsigned lit = *--p;
      assert (VALID_CHECKER_LIT (lit));
      const unsigned not_lit = lit ^ 1;
      assert (values[lit] > 0);
      assert (values[not_lit] < 0);
      values[lit] = values[not_lit] = 0;
    }
  checker->propagated = saved;
  SET_END_OF_STACK (checker->trail, begin);
}

static void
check_line (kissat * solver, checker * checker)
{
  checker->checked++;
  if (checker->inconsistent)
    return;
  if (!checker_propagate (solver, checker))
    {
      LOG3 ("root level checker unit propagations leads to conflict");
      LOG2 ("checker becomes inconsistent");
      checker->inconsistent = true;
      return;
    }
  const unsigned saved = SIZE_STACK (checker->trail);
  signed char *values = checker->values;
  bool satisfied = false;
  unsigned decisions = 0;
  for (all_stack (unsigned, lit, checker->imported))
    {
      signed char lit_value = values[lit];
      if (lit_value < 0)
	continue;
      if (lit_value > 0)
	{
	  satisfied = true;
	  break;
	}
      const unsigned not_lit = lit ^ 1;
      checker_assign (solver, checker, not_lit, &decision_line);
      decisions++;
    }
  checker->decisions += decisions;
  if (!satisfied && checker_propagate (solver, checker))
    {
      kissat_fatal_message_start ();
      fputs ("failed to check clause:\n", stderr);
      for (all_stack (unsigned, lit, checker->imported))
	  fprintf (stderr, "%d ", export_checker (checker, lit));
      fputs ("0\n", stderr);
      fflush (stderr);
      kissat_abort ();
    }
  LOG3 ("checker imported clause unit implied");
  checker_backtrack (checker, saved);
}

void
kissat_add_unchecked_external (kissat * solver, size_t size, int *elits)
{
  LOGINTS3 (size, elits, "adding unchecked external checker");
  checker *checker = solver->checker;
  checker->unchecked++;
  import_external_literals (solver, checker, size, elits);
  insert_imported_if_not_simplified (solver, checker);
}

void
kissat_add_unchecked_internal (kissat * solver, size_t size, unsigned *lits)
{
  LOGLITS3 (size, lits, "adding unchecked internal checker");
  checker *checker = solver->checker;
  checker->unchecked++;
  assert (size <= UINT_MAX);
  import_internal_literals (solver, checker, size, lits);
  insert_imported_if_not_simplified (solver, checker);
}

void
kissat_check_and_add_binary (kissat * solver, unsigned a, unsigned b)
{
  LOGBINARY3 (a, b, "checking and adding internal checker");
  checker *checker = solver->checker;
  assert (VALID_INTERNAL_LITERAL (a));
  assert (VALID_INTERNAL_LITERAL (b));
  import_binary (solver, checker, a, b);
  check_line (solver, checker);
  insert_imported_if_not_simplified (solver, checker);
}

void
kissat_check_and_add_clause (kissat * solver, clause * clause)
{
  LOGCLS3 (clause, "checking and adding internal checker");
  checker *checker = solver->checker;
  import_clause (solver, checker, clause);
  check_line (solver, checker);
  insert_imported_if_not_simplified (solver, checker);
}

void
kissat_check_and_add_empty (kissat * solver)
{
  LOG3 ("checking and adding empty checker clause");
  checker *checker = solver->checker;
  CLEAR_STACK (checker->imported);
  check_line (solver, checker);
  insert_imported_if_not_simplified (solver, checker);
}

void
kissat_check_and_add_internal (kissat * solver, size_t size, unsigned *lits)
{
  LOGLITS3 (size, lits, "checking and adding internal checker");
  checker *checker = solver->checker;
  import_internal_literals (solver, checker, size, lits);
  check_line (solver, checker);
  insert_imported_if_not_simplified (solver, checker);
}

void
kissat_check_and_add_unit (kissat * solver, unsigned a)
{
  LOG3 ("checking and adding internal checker internal unit %u", a);
  checker *checker = solver->checker;
  assert (VALID_INTERNAL_LITERAL (a));
  import_internal_unit (solver, checker, a);
  check_line (solver, checker);
  insert_imported_if_not_simplified (solver, checker);
}

void
kissat_check_shrink_clause (kissat * solver, clause * c,
			    unsigned remove, unsigned keep)
{
  LOGCLS3 (c, "checking and shrinking by %u internal checker", remove);
  checker *checker = solver->checker;
  CLEAR_STACK (checker->imported);
  const value *values = solver->values;
  for (all_literals_in_clause (ilit, c))
    {
      if (ilit == remove)
	continue;
      if (ilit != keep && values[ilit] < 0 && !LEVEL (ilit))
	continue;
      const unsigned lit = import_internal_checker (solver, checker, ilit);
      PUSH_STACK (checker->imported, lit);
    }
  LOGIMPORTED3 ("checker imported internal");
  check_line (solver, checker);
  insert_imported_if_not_simplified (solver, checker);
  import_clause (solver, checker, c);
  remove_line_if_not_redundant (solver, checker);
}

void
kissat_remove_checker_binary (kissat * solver, unsigned a, unsigned b)
{
  LOGBINARY3 (a, b, "removing internal checker");
  checker *checker = solver->checker;
  assert (VALID_INTERNAL_LITERAL (a));
  assert (VALID_INTERNAL_LITERAL (b));
  import_binary (solver, checker, a, b);
  remove_line_if_not_redundant (solver, checker);
}

void
kissat_remove_checker_clause (kissat * solver, clause * clause)
{
  LOGCLS3 (clause, "removing internal checker");
  checker *checker = solver->checker;
  import_clause (solver, checker, clause);
  remove_line_if_not_redundant (solver, checker);
}

bool
kissat_checker_contains_clause (kissat * solver, clause * clause)
{
  checker *checker = solver->checker;
  import_clause (solver, checker, clause);
  size_t size = SIZE_STACK (checker->imported);
  if (line_redundant (solver, checker, size))
    return true;
  return find_line (solver, checker, size, false);
}

void
kissat_remove_checker_external (kissat * solver, size_t size, int *elits)
{
  LOGINTS3 (size, elits, "removing external checker");
  checker *checker = solver->checker;
  import_external_literals (solver, checker, size, elits);
  remove_line_if_not_redundant (solver, checker);
}

void
kissat_remove_checker_internal (kissat * solver, size_t size, unsigned *ilits)
{
  LOGLITS3 (size, ilits, "removing internal checker");
  checker *checker = solver->checker;
  import_internal_literals (solver, checker, size, ilits);
  remove_line_if_not_redundant (solver, checker);
}

void
dump_line (line * line)
{
  printf ("line[%p]", (void *) line);
  for (unsigned i = 0; i < line->size; i++)
    printf (" %u", line->lits[i]);
  fputc ('\n', stdout);
}

void
dump_checker (kissat * solver)
{
  checker *checker = solver->checker;
  printf ("%s\n", checker->inconsistent ? "inconsistent" : "consistent");
  printf ("vars %u\n", checker->vars);
  printf ("size %u\n", checker->size);
  printf ("lines %u\n", checker->lines);
  printf ("hashed %u\n", checker->hashed);
  for (unsigned i = 0; i < SIZE_STACK (checker->trail); i++)
    printf ("trail[%u] %u\n", i, PEEK_STACK (checker->trail, i));
  for (unsigned h = 0; h < checker->hashed; h++)
    for (line * line = checker->table[h]; line; line = line->next)
      dump_line (line);
}

#else
int kissat_check_dummy_to_avoid_warning;
#endif
