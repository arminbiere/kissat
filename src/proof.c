#ifndef NPROOFS

#include "allocate.h"
#include "file.h"
#include "inline.h"

struct proof
{
  bool binary;
  file *file;
  ints line;
  uint64_t added;
  uint64_t deleted;
  uint64_t lines;
  uint64_t literals;
};

void
kissat_init_proof (kissat * solver, file * file, bool binary)
{
  assert (file);
  assert (!solver->proof);
  proof *proof = kissat_calloc (solver, 1, sizeof (struct proof));
  proof->binary = binary;
  proof->file = file;
  solver->proof = proof;
  LOG ("starting to trace %s proof", binary ? "binary" : "non-binary");
}

void
kissat_release_proof (kissat * solver)
{
  proof *proof = solver->proof;
  assert (proof);
  LOG ("stopping to trace proof");
  RELEASE_STACK (proof->line);
  kissat_free (solver, proof, sizeof (struct proof));
  solver->proof = 0;
}

#ifndef QUIET

#include <inttypes.h>

#define PERCENT_LINES(NAME) \
  kissat_percent (proof->NAME, proof->lines)

void
kissat_print_proof_statistics (kissat * solver, bool verbose)
{
  proof *proof = solver->proof;
  PRINT_STAT ("proof_added", proof->added,
	      PERCENT_LINES (added), "%", "per line");
  PRINT_STAT ("proof_bytes", proof->file->bytes,
	      proof->file->bytes / (double) (1 << 20), "MB", "");
  PRINT_STAT ("proof_deleted", proof->deleted,
	      PERCENT_LINES (deleted), "%", "per line");
  if (verbose)
    PRINT_STAT ("proof_lines", proof->lines, 100, "%", "");
  if (verbose)
    PRINT_STAT ("proof_literals", proof->literals,
		kissat_average (proof->literals, proof->lines),
		"", "per line");
}

#endif

static void
import_internal_proof_literal (kissat * solver, proof * proof, unsigned ilit)
{
  int elit = kissat_export_literal (solver, ilit);
  assert (elit);
  PUSH_STACK (proof->line, elit);
  proof->literals++;
}

static void
import_external_proof_literal (kissat * solver, proof * proof, int elit)
{
  assert (elit);
  PUSH_STACK (proof->line, elit);
  proof->literals++;
}

static void
import_internal_proof_binary (kissat * solver, proof * proof,
			      unsigned a, unsigned b)
{
  assert (EMPTY_STACK (proof->line));
  import_internal_proof_literal (solver, proof, a);
  import_internal_proof_literal (solver, proof, b);
}

static void
import_internal_proof_literals (kissat * solver, proof * proof,
				size_t size, unsigned *ilits)
{
  assert (EMPTY_STACK (proof->line));
  assert (size <= UINT_MAX);
  for (size_t i = 0; i < size; i++)
    import_internal_proof_literal (solver, proof, ilits[i]);
}

static void
import_external_proof_literals (kissat * solver, proof * proof,
				size_t size, int *elits)
{
  assert (EMPTY_STACK (proof->line));
  assert (size <= UINT_MAX);
  for (size_t i = 0; i < size; i++)
    import_external_proof_literal (solver, proof, elits[i]);
}

static void
import_proof_clause (kissat * solver, proof * proof, clause * c)
{
  import_internal_proof_literals (solver, proof, c->size, c->lits);
}

static void
print_binary_proof_line (proof * proof)
{
  assert (proof->binary);
  for (all_stack (int, elit, proof->line))
    {
      unsigned x = 2u * ABS (elit) + (elit < 0);
      unsigned char ch;
      while (x & ~0x7f)
	{
	  ch = (x & 0x7f) | 0x80;
	  kissat_putc (proof->file, ch);
	  x >>= 7;
	}
      kissat_putc (proof->file, (unsigned char) x);
    }
  kissat_putc (proof->file, 0);
}

static void
print_non_binary_proof_line (proof * proof)
{
  assert (!proof->binary);
  char buffer[16];
  char *end_of_buffer = buffer + sizeof buffer;
  *--end_of_buffer = 0;
  for (all_stack (int, elit, proof->line))
    {
      char *p = end_of_buffer;
      assert (!*p);
      assert (elit);
      assert (elit != INT_MIN);
      unsigned eidx;
      if (elit < 0)
	{
	  kissat_putc (proof->file, '-');
	  eidx = -elit;
	}
      else
	eidx = elit;
      for (unsigned tmp = eidx; tmp; tmp /= 10)
	*--p = '0' + (tmp % 10);
      while (p != end_of_buffer)
	kissat_putc (proof->file, *p++);
      kissat_putc (proof->file, ' ');
    }
  kissat_putc (proof->file, '0');
  kissat_putc (proof->file, '\n');
}

static void
print_proof_line (proof * proof)
{
  proof->lines++;
  if (proof->binary)
    print_binary_proof_line (proof);
  else
    print_non_binary_proof_line (proof);
  CLEAR_STACK (proof->line);
#ifndef NDEBUG
  fflush (proof->file->file);
#endif
}

static void
print_added_proof_line (proof * proof)
{
  proof->added++;
  if (proof->binary)
    kissat_putc (proof->file, 'a');
  print_proof_line (proof);
}

static void
print_delete_proof_line (proof * proof)
{
  proof->deleted++;
  kissat_putc (proof->file, 'd');
  if (!proof->binary)
    kissat_putc (proof->file, ' ');
  print_proof_line (proof);
}

void
kissat_add_binary_to_proof (kissat * solver, unsigned a, unsigned b)
{
  proof *proof = solver->proof;
  assert (proof);
  import_internal_proof_binary (solver, proof, a, b);
  print_added_proof_line (proof);
}

void
kissat_add_clause_to_proof (kissat * solver, clause * c)
{
  proof *proof = solver->proof;
  assert (proof);
  import_proof_clause (solver, proof, c);
  print_added_proof_line (proof);
}

void
kissat_add_empty_to_proof (kissat * solver)
{
  proof *proof = solver->proof;
  assert (proof);
  assert (EMPTY_STACK (proof->line));
  print_added_proof_line (proof);
}

void
kissat_add_lits_to_proof (kissat * solver, size_t size, unsigned *ilits)
{
  proof *proof = solver->proof;
  assert (proof);
  import_internal_proof_literals (solver, proof, size, ilits);
  print_added_proof_line (proof);
}

void
kissat_add_unit_to_proof (kissat * solver, unsigned ilit)
{
  proof *proof = solver->proof;
  assert (proof);
  assert (EMPTY_STACK (proof->line));
  import_internal_proof_literal (solver, proof, ilit);
  print_added_proof_line (proof);
}

void
kissat_shrink_clause_in_proof (kissat * solver, clause * c,
			       unsigned remove, unsigned keep)
{
  proof *proof = solver->proof;
  const value *values = solver->values;
  assert (EMPTY_STACK (proof->line));
  for (all_literals_in_clause (ilit, c))
    {
      if (ilit == remove)
	continue;
      if (ilit != keep && values[ilit] < 0 && !LEVEL (ilit))
	continue;
      import_internal_proof_literal (solver, proof, ilit);
    }
  print_added_proof_line (proof);
  import_proof_clause (solver, proof, c);
  print_delete_proof_line (proof);
}

void
kissat_delete_binary_from_proof (kissat * solver, unsigned a, unsigned b)
{
  proof *proof = solver->proof;
  assert (proof);
  import_internal_proof_binary (solver, proof, a, b);
  print_delete_proof_line (proof);
}

void
kissat_delete_clause_from_proof (kissat * solver, clause * c)
{
  proof *proof = solver->proof;
  assert (proof);
  import_proof_clause (solver, proof, c);
  print_delete_proof_line (proof);
}

void
kissat_delete_external_from_proof (kissat * solver, size_t size, int *elits)
{
  proof *proof = solver->proof;
  assert (proof);
  import_external_proof_literals (solver, proof, size, elits);
  print_delete_proof_line (proof);
}

void
kissat_delete_internal_from_proof (kissat * solver,
				   size_t size, unsigned *ilits)
{
  proof *proof = solver->proof;
  assert (proof);
  import_internal_proof_literals (solver, proof, size, ilits);
  print_delete_proof_line (proof);
}

#else
int kissat_proof_dummy_to_avoid_warning;
#endif
