#include "test.h"

#if defined(NDEBUG) || !defined(NOPTIONS)

#include "../src/collect.h"
#include "../src/dense.h"
#include "../src/flags.h"
#include "../src/import.h"
#include "../src/propsearch.h"
#include "../src/trail.h"

#define REDUNDANT_LITERAL 2
#define IRREDUNDANT_LITERAL 4

static void
flush_unit (kissat * solver)
{
  clause *conflict = kissat_search_propagate (solver);
  assert (!conflict);
  kissat_flush_trail (solver);
}

static void
add_unit_to_satisfy_redundant_clause (kissat * solver)
{
  if (solver->values[REDUNDANT_LITERAL])
    return;
  kissat_assign_unit (solver, REDUNDANT_LITERAL);
  flush_unit (solver);
}

static void
add_unit_to_satisfy_irredundant_clause (kissat * solver)
{
  if (solver->values[IRREDUNDANT_LITERAL])
    return;
  kissat_assign_unit (solver, IRREDUNDANT_LITERAL);
  flush_unit (solver);
}

static void
just_import_and_activate_four_variables (kissat * solver)
{
  int ilit;
  ilit = kissat_import_literal (solver, 1);
  assert (ilit == 0);
  kissat_activate_literal (solver, ilit);
  ilit = kissat_import_literal (solver, 2);
  assert (ilit == 2);
  kissat_activate_literal (solver, ilit);
  ilit = kissat_import_literal (solver, 3);
  assert (ilit == 4);
  kissat_activate_literal (solver, ilit);
  ilit = kissat_import_literal (solver, 4);
  assert (ilit == 6);
  kissat_activate_literal (solver, ilit);
}

static void
add_large_redundant_clause (kissat * solver)
{
  assert (solver->vars == 4);
  PUSH_STACK (solver->clause.lits, 0);
  PUSH_STACK (solver->clause.lits, REDUNDANT_LITERAL);
  PUSH_STACK (solver->clause.lits, 6);
  kissat_new_redundant_clause (solver, 2);
}

static void
add_large_irredundant_clause (kissat * solver)
{
  assert (solver->vars == 4);
  PUSH_STACK (solver->clause.lits, 0);
  PUSH_STACK (solver->clause.lits, IRREDUNDANT_LITERAL);
  PUSH_STACK (solver->clause.lits, 6);
  kissat_new_irredundant_clause (solver);
}

static void
mark_redundant_clauses_as_garbage (kissat * solver)
{
  for (all_clauses (c))
    if (c->redundant)
      kissat_mark_clause_as_garbage (solver, c);
}

static void
mark_irredundant_clauses_as_garbage (kissat * solver)
{
  for (all_clauses (c))
    if (!c->redundant)
      kissat_mark_clause_as_garbage (solver, c);
}

#define IFBIT(BIT,FUNCTION) \
do { \
  assert (BIT < bits); \
  if ((i & (1 << BIT))) \
    FUNCTION (solver); \
} while (0)

static void
test_collect (void)
{
  const unsigned bits = 14;
  for (unsigned i = 0; i < (1u << bits); i++)
    {
      if (!(i & ((i << 7) | (i << 9) | (i << 11))))
	continue;

      kissat *solver = kissat_init ();
      tissat_init_solver (solver);
#if !defined(NDEBUG)
      solver->options.check = 0;
#endif
      just_import_and_activate_four_variables (solver);

      IFBIT (0, add_large_redundant_clause);
      IFBIT (1, add_large_irredundant_clause);
      IFBIT (2, add_large_redundant_clause);
      IFBIT (3, add_large_irredundant_clause);
      IFBIT (4, add_large_redundant_clause);

      bool redundant_unit;
      bool irredundant_unit;

      if (i & (1 << 13))
	{
	  redundant_unit = irredundant_unit = false;
	  IFBIT (5, mark_redundant_clauses_as_garbage);
	  IFBIT (6, mark_irredundant_clauses_as_garbage);
	}
      else
	{
	  redundant_unit = ((i & (i << 5)) != 0);
	  irredundant_unit = ((i & (i << 6)) != 0);
	  IFBIT (5, add_unit_to_satisfy_redundant_clause);
	  IFBIT (6, add_unit_to_satisfy_irredundant_clause);
	}

      if (i & (i << 7))
	{
	  bool compact = !(i & (i << 8));
	  kissat_sparse_collect (solver, compact, 0);
	}
      else if (i & (i << 8))
	{
	  if (!redundant_unit)
	    add_unit_to_satisfy_redundant_clause (solver);
	  if (!irredundant_unit)
	    add_unit_to_satisfy_irredundant_clause (solver);
	  redundant_unit = irredundant_unit = true;
	}

      if (i & (i << 9))
	{
	  kissat_enter_dense_mode (solver, 0, 0);
	  kissat_dense_collect (solver);
	  bool flush = !(i & (i << 10));
	  kissat_resume_sparse_mode (solver, flush, 0, 0);
	}
      else if (i & (i << 10))
	{
	  if (!redundant_unit)
	    add_unit_to_satisfy_redundant_clause (solver);
	  if (!irredundant_unit)
	    add_unit_to_satisfy_irredundant_clause (solver);
	}

      if (i & (i << 11))
	{
	  bool compact = !(i & (i << 12));
	  kissat_sparse_collect (solver, compact, 0);
	}

      kissat_release (solver);
    }
}

void
tissat_schedule_collect (void)
{
  SCHEDULE_FUNCTION (test_collect);
}

#else

void
tissat_schedule_collect (void)
{
}

#endif
