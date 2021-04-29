#include "allocate.h"
#include "definition.h"
#include "gates.h"
#include "inline.h"
#include "kitten.h"
#include "print.h"

typedef struct definition_extractor definition_extractor;

struct definition_extractor
{
  unsigned lit;
  kissat *solver;
  watches *watches[2];
};

static void
traverse_definition_core (void *state, unsigned id)
{
  definition_extractor *extractor = state;
  kissat *solver = extractor->solver;
  watch watch;
  watches *watches0 = extractor->watches[0];
  watches *watches1 = extractor->watches[1];
  const size_t size_watches0 = SIZE_WATCHES (*watches0);
  assert (size_watches0 <= UINT_MAX);
  unsigned sign;
  if (id < size_watches0)
    {
      watch = BEGIN_WATCHES (*watches0)[id];
      LOGWATCH (extractor->lit, watch, "gate[0]");
      sign = 0;
    }
  else
    {
      unsigned tmp = id - size_watches0;
#ifndef NDEBUG
      const size_t size_watches1 = SIZE_WATCHES (*watches1);
      assert (size_watches1 <= UINT_MAX);
      assert (tmp < size_watches1);
#endif
      watch = BEGIN_WATCHES (*watches1)[tmp];
      LOGWATCH (NOT (extractor->lit), watch, "gate[1]");
      sign = 1;
    }
  PUSH_STACK (solver->gates[sign], watch);
}

#if !defined (NDEBUG) || !defined(NPROOFS)

typedef struct lemma_extractor lemma_extractor;

struct lemma_extractor
{
  kissat *solver;
  unsigned lemmas;
  unsigned unit;
};

static void
traverse_one_sided_core_lemma (void *state,
			       size_t msize, const unsigned *mlits)
{
  lemma_extractor *extractor = state;
  kissat *solver = extractor->solver;
  const unsigned unit = extractor->unit;
  unsigneds *added = &solver->added;
  unsigneds *analyzed = &solver->analyzed;
  assert (extractor->lemmas || EMPTY_STACK (*added));
  if (msize)
    {
      assert (msize <= UINT_MAX);
      const size_t size = msize + 1;
      PUSH_STACK (*added, size);
      const size_t offset = SIZE_STACK (*added);
      PUSH_STACK (*added, unit);
      const unsigned *end = mlits + msize;
      for (const unsigned *p = mlits; p != end; p++)
	{
	  const unsigned mlit = *p;
	  const unsigned midx = mlit / 2;
	  const unsigned idx = PEEK_STACK (*analyzed, midx);
	  const unsigned lit = LIT (idx) + (mlit & 1);
	  PUSH_STACK (*added, lit);
	}
      unsigned *lits = &PEEK_STACK (*added, offset);
      assert (offset + size + 1 == SIZE_STACK (*added));
      CHECK_AND_ADD_LITS (size + 1, lits);
      ADD_LITS_TO_PROOF (size + 1, lits);
    }
  else
    {
      kissat_learned_unit (solver, unit);
      const unsigned *end = END_STACK (*added);
      unsigned *begin = BEGIN_STACK (*added);
      for (unsigned *p = begin, size; p != end; p += size)
	{
	  size = *p++;
	  assert (p + size <= end);
	  REMOVE_CHECKER_LITS (size, p);
	  DELETE_LITS_FROM_PROOF (size, p);
	}
      CLEAR_STACK (*added);
    }
  extractor->lemmas++;
}

#endif

static void
add_mapped_literal (kissat * solver, unsigned *const map,
		    unsigneds * mclause, unsigneds * analyzed,
		    unsigned lit, unsigned *vars_ptr)
{
  const unsigned idx = IDX (lit);
  if (!map[idx])
    {
      PUSH_STACK (*analyzed, idx);
      unsigned vars = *vars_ptr;
      *vars_ptr = ++vars;
      map[idx] = vars;
      assert (vars == SIZE_STACK (*analyzed));
      assert (PEEK_STACK (*analyzed, vars - 1) == idx);
    }
  const unsigned midx = map[idx] - 1;
  const unsigned mlit = 2 * midx + (lit & 1);
  PUSH_STACK (*mclause, mlit);
}

bool
kissat_find_definition (kissat * solver, unsigned lit)
{
  if (!GET_OPTION (definitions))
    return false;
  START (definition);
  struct kitten *kitten = solver->kitten;
  assert (kitten);
  kitten_clear (kitten);
  const unsigned not_lit = NOT (lit);
  definition_extractor extractor;
  extractor.lit = lit;
  extractor.solver = solver;
  extractor.watches[0] = &WATCHES (lit);
  extractor.watches[1] = &WATCHES (not_lit);
  unsigneds mclause;
  INIT_STACK (mclause);
  kitten_track_antecedents (kitten);
  unsigned vars = 0;
  unsigned *const map = solver->map;
  unsigned exported = 0;
  assert (EMPTY_STACK (solver->analyzed));
#if !defined(QUIET) || !defined(NDEBUG)
  size_t occs[2] = { 0, 0 };
#endif
  unsigneds *analyzed = &solver->analyzed;
  for (unsigned sign = 0; sign < 2; sign++)
    {
      for (all_binary_large_watches (watch, *extractor.watches[sign]))
	{
	  assert (EMPTY_STACK (mclause));
	  if (watch.type.binary)
	    {
	      const unsigned other = watch.binary.lit;
	      add_mapped_literal (solver, map,
				  &mclause, analyzed, other, &vars);
	    }
	  else
	    {
	      const reference ref = watch.large.ref;
	      clause *c = kissat_dereference_clause (solver, ref);
	      for (all_literals_in_clause (other, c))
		{
		  if (sign)
		    {
		      if (other == not_lit)
			continue;
		      assert (other != lit);
		    }
		  else
		    {
		      if (other == lit)
			continue;
		      assert (other != not_lit);
		    }
		  add_mapped_literal (solver, map,
				      &mclause, analyzed, other, &vars);
		}
	    }
	  assert (SIZE_STACK (mclause) <= UINT_MAX);
	  unsigned size = SIZE_STACK (mclause);
	  const unsigned *const lits = BEGIN_STACK (mclause);
	  kitten_clause (kitten, exported, size, lits);
	  CLEAR_STACK (mclause);
#if !defined(QUIET) || !defined(NDEBUG)
	  occs[sign]++;
#endif
	  exported++;
	}
    }
  bool res = false;
  LOG ("environment of literal %s has %u variables", LOGLIT (lit), vars);
  LOG ("exported %u = %zu + %zu environment clauses to sub-solver",
       exported, occs[0], occs[1]);
  INC (definitions_checked);
  int status = kitten_solve (kitten);
  if (status == 20)
    {
      INC (definitions_extracted);
      LOG ("sub-solver result UNSAT shows definition exists");
      uint64_t learned;
      unsigned reduced = kitten_compute_clausal_core (kitten, &learned);
      LOG ("1st sub-solver core of size %u original clauses out of %u",
	   reduced, exported);
      for (int i = 2; i <= GET_OPTION (definitioncores); i++)
	{
	  kitten_shrink_to_clausal_core (kitten);
	  kitten_shuffle (kitten);
#ifndef NDEBUG
	  int tmp =
#endif
	    kitten_solve (kitten);
	  assert (tmp == 20);
#ifndef NDEBUG
	  unsigned previous = reduced;
#endif
	  reduced = kitten_compute_clausal_core (kitten, &learned);
	  LOG ("%s sub-solver core of size %u original clauses out of %u",
	       FORMAT_ORDINAL (i), reduced, exported);
	  assert (reduced <= previous);
#if defined(QUIET) && defined(NDEBUG)
	  (void) reduced;
#endif
	}
      kitten_traverse_clausal_core (kitten, &extractor,
				    traverse_definition_core);
      size_t size[2];
      size[0] = SIZE_STACK (solver->gates[0]);
      size[1] = SIZE_STACK (solver->gates[1]);
#if !defined(QUIET) || !defined(NDEBUG)
      assert (reduced == size[0] + size[1]);
#ifdef METRICS
      kissat_extremely_verbose (solver,
				"definition extracted[%" PRIu64 "] "
				"size %u = %zu + %zu clauses %.0f%% "
				"of %u = %zu + %zu (checked %" PRIu64 ")",
				solver->statistics.definitions_extracted,
				reduced, size[0], size[1],
				kissat_percent (reduced, exported),
				exported, occs[0], occs[1],
				solver->statistics.definitions_checked);
#else
      kissat_extremely_verbose (solver,
				"definition extracted with core "
				"size %u = %zu + %zu clauses %.0f%% "
				"of %u = %zu + %zu",
				reduced, size[0], size[1],
				kissat_percent (reduced, exported),
				exported, occs[0], occs[1]);
#endif
#endif
      unsigned unit = INVALID_LIT;
      if (!size[0])
	{
	  unit = not_lit;
	  assert (size[1]);
	}
      else if (!size[1])
	unit = lit;

      if (unit != INVALID_LIT)
	{
	  INC (definition_units);

	  kissat_extremely_verbose (solver, "one sided core "
				    "definition extraction yields "
				    "failed literal");
#if !defined (NDEBUG) || !defined(NPROOFS)
	  if (false
#ifndef NDEBUG
	      || GET_OPTION (check) > 1
#endif
#ifndef NPROOFS
	      || solver->proof
#endif
	    )
	    {
	      lemma_extractor extractor;
	      extractor.solver = solver;
	      extractor.unit = unit;
	      extractor.lemmas = 0;
	      kitten_traverse_core_lemmas (kitten, &extractor,
					   traverse_one_sided_core_lemma);
	    }
	  else
#endif
	    kissat_learned_unit (solver, unit);
	}
      solver->gate_eliminated = GATE_ELIMINATED (definitions);
      solver->resolve_gate = true;
      res = true;
    }
  else
    LOG ("sub-solver failed to show that definition exists");
  RELEASE_STACK (mclause);
  for (all_stack (unsigned, idx, solver->analyzed))
      map[idx] = 0;
  CLEAR_STACK (solver->analyzed);
  STOP (definition);
  return res;
}
