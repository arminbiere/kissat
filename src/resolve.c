#include "eliminate.h"
#include "gates.h"
#include "inline.h"
#include "resolve.h"

#include <inttypes.h>

static unsigned
occurrences_literal (kissat * solver, unsigned lit, bool * update)
{
  assert (!solver->watching);

  watches *watches = &WATCHES (lit);
  LOG ("literal %s has %u watches", LOGLIT (lit), watches->size);

  const unsigned clslim = solver->bounds.eliminate.clause_size;

  watch *begin = BEGIN_WATCHES (*watches), *q = begin;
  const watch *end = END_WATCHES (*watches), *p = q;

  const value *values = solver->values;
  const word *arena = BEGIN_STACK (solver->arena);

  bool failed = false;
  unsigned res = 0;

  while (p != end)
    {
      watch head = *q++ = *p++;
      if (head.type.binary)
	{
	  const unsigned other = head.binary.lit;
	  const value value = values[other];
	  assert (value >= 0);
	  if (value > 0)
	    {
	      kissat_eliminate_binary (solver, lit, other);
	      q--;
	    }
	  else
	    res++;
	}
      else
	{
	  const reference ref = head.large.ref;
	  assert (ref < SIZE_STACK (solver->arena));
	  clause *c = (struct clause *) (arena + ref);
	  if (c->garbage)
	    q--;
	  else if (c->size > clslim)
	    {
	      LOG ("literal %s watches too long clause of size %u",
		   LOGLIT (lit), c->size);
	      failed = true;
	      break;
	    }
	  else
	    res++;
	}
    }
  while (p != end)
    *q++ = *p++;
  SET_END_OF_WATCHES (*watches, q);
  if (failed)
    return UINT_MAX;
  if (res != watches->size)
    {
      *update = true;
      LOG ("literal %s actually occurs only %u times", LOGLIT (lit), res);
    }
  return res;
}

static inline clause *
watch_to_clause (kissat * solver,
		 const value * values, const word * arena,
		 clause * tmp, unsigned lit, watch watch)
{
  clause *res;
  if (watch.type.binary)
    {
      const unsigned other = watch.binary.lit;
      assert (!values[other]);
      tmp->lits[0] = lit;
      tmp->lits[1] = other;
      res = tmp;
    }
  else
    {
      const reference ref = watch.large.ref;
      assert (ref < SIZE_STACK (solver->arena));
      res = (struct clause *) (arena + ref);
      assert (!res->garbage);
    }
#ifdef NDEBUG
  (void) solver;
  (void) values;
#endif
  return res;
}

static bool
generate_resolvents (kissat * solver, unsigned lit,
		     statches * watches0, statches * watches1,
		     uint64_t * resolved_ptr, uint64_t limit)
{
  const unsigned not_lit = NOT (lit);
  unsigned resolved = *resolved_ptr;
  bool failed = false;

  clause tmp0, tmp1;
  memset (&tmp0, 0, sizeof tmp0);
  memset (&tmp1, 0, sizeof tmp1);
  tmp0.size = tmp1.size = 2;

  const word *arena = BEGIN_STACK (solver->arena);
  const value *values = solver->values;
  value *marks = solver->marks;

  const unsigned clslim = solver->bounds.eliminate.clause_size;

  for (all_stack (watch, watch0, *watches0))
    {
      clause *c = watch_to_clause (solver, values, arena, &tmp0, lit, watch0);

      for (all_literals_in_clause (other, c))
	{
	  if (other == lit)
	    continue;
	  assert (!marks[other]);
	  marks[other] = 1;
	}

      for (all_stack (watch, watch1, *watches1))
	{
	  clause *d =
	    watch_to_clause (solver, values, arena, &tmp1, not_lit, watch1);

	  LOGCLS (c, "first %s antecedent", LOGLIT (lit));
	  LOGCLS (d, "second %s antecedent", LOGLIT (not_lit));

	  bool satisfied_or_tautological = false;
	  size_t saved = SIZE_STACK (solver->resolvents);

	  INC (resolutions);

	  for (all_literals_in_clause (other, d))
	    {
	      if (other == not_lit)
		continue;
	      if (marks[other])
		{
		  LOG2 ("dropping repeated %s literal", LOGLIT (other));
		  continue;
		}
	      const unsigned not_other = NOT (other);
	      if (marks[not_other])
		{
		  LOG ("resolvent tautological on %s and %s "
		       "with second %s antecedent",
		       LOGLIT (NOT (other)), LOGLIT (other),
		       LOGLIT (not_lit));
		  satisfied_or_tautological = true;
		  break;
		}
	      const value value = values[other];
	      if (value < 0)
		{
		  LOG2 ("dropping falsified literal %s", LOGLIT (other));
		  continue;
		}
	      assert (!value);
	      LOG2 ("including unassigned literal %s", LOGLIT (other));
	      PUSH_STACK (solver->resolvents, other);
	    }

	  if (satisfied_or_tautological)
	    {
	      RESIZE_STACK (solver->resolvents, saved);
	      continue;
	    }

	  if (++resolved > limit)
	    {
	      LOG ("limit of %u resolvent exceeded", limit);
	      failed = true;
	      break;
	    }

	  for (all_literals_in_clause (other, c))
	    {
	      if (other == lit)
		continue;
	      const value value = values[lit];
	      assert (value <= 0);
	      if (value < 0)
		{
		  LOG2 ("dropping falsified literal %s", LOGLIT (other));
		  continue;
		}
	      PUSH_STACK (solver->resolvents, other);
	    }

	  size_t size_resolvent = SIZE_STACK (solver->resolvents) - saved;
	  LOGLITS (size_resolvent,
		   BEGIN_STACK (solver->resolvents) + saved, "resolvent");

	  if (size_resolvent > clslim)
	    {
	      LOG ("resolvent size limit exceeded");
	      failed = true;
	      break;
	    }

	  PUSH_STACK (solver->resolvents, INVALID_LIT);
	}

      for (all_literals_in_clause (other, c))
	{
	  if (other == lit)
	    continue;
	  assert (marks[other] == 1);
	  marks[other] = 0;
	}

      if (failed)
	break;
    }

  *resolved_ptr = resolved;

  return !failed;
}

bool
kissat_generate_resolvents (kissat * solver, unsigned idx, unsigned *lit_ptr)
{
  unsigned lit = LIT (idx);
  unsigned not_lit = NOT (lit);

  bool update = false;
  uint64_t limit;

  {
    unsigned pos_count = occurrences_literal (solver, lit, &update);
    unsigned neg_count = occurrences_literal (solver, not_lit, &update);

    if (pos_count > neg_count)
      {
	SWAP (unsigned, lit, not_lit);
	SWAP (size_t, pos_count, neg_count);
      }

    const unsigned occlim = solver->bounds.eliminate.occurrences;
    if (pos_count && neg_count > occlim)
      {
	LOG ("not elimination of variable %u since limit hit", idx);
	return false;
      }

    limit = pos_count + (uint64_t) neg_count;
    if (pos_count)
      {
	limit += solver->bounds.eliminate.additional_clauses;
	LOG ("trying elimination of variable %u limit %" PRIu64, idx, limit);
      }
    else
      LOG ("eliminating pure literal %u (variable %u)", lit, idx);
  }

  const bool gates = kissat_find_gates (solver, lit);
  kissat_get_antecedents (solver, lit);

  uint64_t resolved = 0;
  bool failed = false;

  statches *gates0 = &solver->gates[0];
  statches *gates1 = &solver->gates[1];
  statches *antecedents0 = &solver->antecedents[0];
  statches *antecedents1 = &solver->antecedents[1];

  if (gates)
    {
      LOG ("resolving gates[0] against antecedents[1] clauses");
      if (!generate_resolvents (solver, lit,
				gates0, antecedents1, &resolved, limit))
	failed = true;
      else
	{
	  LOG ("resolving gates[1] against antecedents[0] clauses");
	  if (!generate_resolvents (solver, not_lit,
				    gates1, antecedents0, &resolved, limit))
	    failed = true;
	  else if (solver->resolve_gate)
	    {
	      LOG ("need to resolved gates[0] against gates[1] too");
	      if (!generate_resolvents (solver, lit,
					gates0, gates1, &resolved, limit))
		failed = true;
	    }
	}
    }
  else
    {
      LOG ("no gate extracted thus resolving all clauses");
      if (!generate_resolvents (solver, lit,
				antecedents0, antecedents1, &resolved, limit))
	failed = true;
    }

  CLEAR_STACK (*gates0);
  CLEAR_STACK (*gates1);
  CLEAR_STACK (*antecedents0);
  CLEAR_STACK (*antecedents1);

  if (failed)
    {
      const unsigned idx = IDX (lit);
      LOG ("elimination of variable %u failed", idx);
      CLEAR_STACK (solver->resolvents);
      if (update)
	kissat_update_variable_score (solver, &solver->schedule, idx);
      return false;
    }

  LOG ("resolved %" PRIu64 " resolvents", resolved);
  *lit_ptr = lit;

  return true;
}
