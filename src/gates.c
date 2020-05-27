#include "ands.h"
#include "eliminate.h"
#include "equivalences.h"
#include "gates.h"
#include "ifthenelse.h"
#include "inline.h"
#include "xors.h"

size_t
kissat_mark_binaries (kissat * solver, unsigned lit)
{
  value *marks = solver->marks;
  size_t res = 0;
  watches *watches = &WATCHES (lit);
  for (all_binary_large_watches (watch, *watches))
    {
      if (!watch.type.binary)
	continue;
      const unsigned other = watch.binary.lit;
      assert (!solver->values[other]);
      if (marks[other])
	continue;
      marks[other] = 1;
      res++;
    }
  return res;
}

void
kissat_unmark_binaries (kissat * solver, unsigned lit)
{
  value *marks = solver->marks;
  watches *watches = &WATCHES (lit);
  for (all_binary_large_watches (watch, *watches))
    if (watch.type.binary)
      marks[watch.binary.lit] = 0;
}

bool
kissat_find_gates (kissat * solver, unsigned lit)
{
  solver->gate_eliminated = 0;
  solver->resolve_gate = false;
  if (!GET_OPTION (extract))
    return false;
  const unsigned not_lit = NOT (lit);
  if (!WATCHES (not_lit).size)
    return false;
  if (kissat_find_equivalence_gate (solver, lit))
    return true;
  if (kissat_find_and_gate (solver, lit, 0))
    return true;
  if (kissat_find_and_gate (solver, not_lit, 1))
    return true;
  if (kissat_find_if_then_else_gate (solver, lit, 0))
    return true;
  if (kissat_find_if_then_else_gate (solver, not_lit, 1))
    return true;
  if (kissat_find_xor_gate (solver, lit, 0))
    return true;
  if (kissat_find_xor_gate (solver, not_lit, 1))
    return true;
  return false;
}

static void
get_antecedents (kissat * solver, unsigned lit, unsigned negative)
{
  assert (!solver->watching);
  assert (!negative || negative == 1);

  statches *gates = solver->gates + negative;
  watches *watches = &WATCHES (lit);

  statches *antecedents = solver->antecedents + negative;
  assert (EMPTY_STACK (*antecedents));

  const watch *begin_gates = BEGIN_STACK (*gates);
  const watch *end_gates = END_STACK (*gates);
  const watch *g = begin_gates;

  const watch *begin_watches = BEGIN_WATCHES (*watches);
  const watch *end_watches = END_WATCHES (*watches);
  const watch *w = begin_watches;

  while (w != end_watches)
    {
      const watch watch = *w++;
      if (g != end_gates && g->raw == watch.raw)
	g++;
      else
	PUSH_STACK (*antecedents, watch);
    }

  assert (g == end_gates);
#ifdef LOGGING
  size_t size_gates = SIZE_STACK (*gates);
  size_t size_antecedents = SIZE_STACK (*antecedents);
  LOG ("got %zu antecedent %.0f%% and %zu gate clauses %.0f%% "
       "out of %zu watches of literal %s",
       size_antecedents, kissat_percent (size_antecedents, watches->size),
       size_gates, kissat_percent (size_gates, watches->size),
       watches->size, LOGLIT (lit));
#endif
}

void
kissat_get_antecedents (kissat * solver, unsigned lit)
{
  get_antecedents (solver, lit, 0);
  get_antecedents (solver, NOT (lit), 1);
}
