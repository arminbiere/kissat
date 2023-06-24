#include "parse.h"
#include "collect.h"
#include "internal.h"
#include "print.h"
#include "profile.h"
#include "resize.h"

#include <ctype.h>
#include <inttypes.h>

#define size_buffer (1u << 20)

struct read_buffer {
  unsigned char chars[size_buffer];
  size_t pos, end;
};

typedef struct read_buffer read_buffer;

static size_t fill_buffer (read_buffer *buffer, file *file) {
  buffer->pos = 0;
  buffer->end = kissat_read (file, buffer->chars, size_buffer);
  return buffer->end;
}

// clang-format off

static inline int
next (read_buffer *, file *, uint64_t *) ATTRIBUTE_ALWAYS_INLINE;

static inline bool faster_is_digit (int ch) ATTRIBUTE_ALWAYS_INLINE;

// clang-format off

static inline int
next (read_buffer * buffer, file * file, uint64_t * lineno_ptr)
{
  if (buffer->pos == buffer->end && !fill_buffer (buffer, file))
    return EOF;
  int ch = buffer->chars[buffer->pos++];
  if (ch == '\n')
    *lineno_ptr += 1;
  return ch;
}

#define NEXT() next (&buffer, file, &lineno)

#define NONL(STR) \
do { \
  if (ch == '\n') \
    { \
      assert (lineno > 0); \
      lineno--; \
    } \
  *lineno_ptr = lineno; \
  return STR; \
} while (0)

#define TRY_RELAXED_PARSING "(try '--relaxed' parsing)"

static inline bool
faster_is_digit (int ch)
{
  return '0' <= ch && ch <= '9';
}

#define ISDIGIT(CH) faster_is_digit (CH)

static const char *
parse_dimacs (kissat * solver, file * file,
              strictness strict, uint64_t * lineno_ptr, int * max_var_ptr)
{
  read_buffer buffer;
  buffer.pos = buffer.end = 0;
  uint64_t lineno = *lineno_ptr = 1;
  bool first = true;
  int ch;
  for (;;)
    {
      ch = NEXT ();
      if (ch == 'p')
	break;
      else if (ch == EOF)
	{
	  if (first)
	    return "empty file";
	  else
	    return "end-of-file before header";
	}
      first = false;
      if (ch == '\r')
	{
	  ch = NEXT ();
	  if (ch != '\n')
	    return "expected new-line after carriage-return";
	  if (strict == PEDANTIC_PARSING)
	    NONL ("unexpected empty line");
	}
      else if (ch == '\n')
	{
	  if (strict == PEDANTIC_PARSING)
	    NONL ("unexpected empty line");
	}
      else if (ch == 'c')
	{
	START:
	  ch = NEXT ();
	  if (ch == '\n')
	    continue;
	  else if (ch == '\r')
	    {
	      ch = NEXT ();
	      if (ch != '\n')
		return "expected new-line after carriage-return";
	      continue;
	    }
	  else if (ch == EOF)
	    return "end-of-file in header comment";
	  else if (ch == ' ' || ch == '\t')
	    goto START;
#if !defined(NOPTIONS) && defined(EMBEDDED)
	  else if (ch == '-' && GET_OPTION (embedded))
	    {
	      ch = NEXT ();
	      if (ch != '-')
		goto COMPLETE;

	      char name[32];
	      unsigned pos = 0;
	      while ('a' <= (ch = NEXT ()) && ch <= 'z')
		{
		  assert (pos < sizeof name);
		  name[pos++] = ch;
		  if (pos == sizeof name)
		    goto COMPLETE;
		}
	      if (ch == '\r')
		{
		  ch = NEXT ();
		  if (ch != '\n')
		    return "expected new-line after carriage-return";
		}
	      if (ch == '\n')
		continue;
	      if (ch != '=')
		goto COMPLETE;
	      assert (pos < sizeof name);
	      name[pos++] = 0;

	      pos = 0;
	      ch = NEXT ();
	      int sign;
	      if (ch == '-')
		{
		  ch = NEXT ();
		  sign = -1;
		}
	      else
		sign = 1;
	      if (!ISDIGIT (ch))
		goto COMPLETE;
	      int arg = ch - '0';
	      while (ISDIGIT (ch = NEXT ()))
		{
		  if (INT_MAX / 10 < arg)
		    goto COMPLETE;
		  arg *= 10;
		  int digit = ch - '0';
		  if (INT_MAX - digit < arg)
		    goto COMPLETE;
		  arg += digit;
		}
	      while (ch == ' ' || ch == '\t')
		ch = NEXT ();
	      if (ch == '\r')
		{
		  ch = NEXT ();
		  if (ch != '\n')
		    return "expected new-line after carriage-return";
		}
	      if (ch != '\n')
		goto COMPLETE;
	      arg *= sign;
	      const opt *const opt = kissat_options_has (name);
	      if (opt)
		{
		  (void) kissat_options_set_opt (&solver->options, opt, arg);
		  kissat_verbose (solver,
				  "parsed embedded option '--%s=%d'",
				  name, arg);
		}
	      else
		kissat_warning (solver,
				"invalid embedded option '--%s=%d'",
				name, arg);
	      continue;
	    }
	  else
#endif
	    {
	      while ((ch = NEXT ()) != '\n')
#if !defined(NOPTIONS) && defined(EMBEDDED)
	      COMPLETE:
#endif
		if (ch == EOF)
		  return "end-of-file in header comment";
		else if (ch == '\r')
		  {
		    ch = NEXT ();
		    if (ch != '\n')
		      return "expected new-line after carriage-return";
		    break;
		  }
	    }
	}
      else
	return "expected 'c' or 'p' at start of line";
    }
  assert (ch == 'p');
  ch = NEXT ();
  if (ch != ' ')
    NONL ("expected space after 'p'");
  ch = NEXT ();
  if (strict != PEDANTIC_PARSING)
    {
      while (ch == ' ' || ch == '\t')
	ch = NEXT ();
    }
  if (ch != 'c')
    NONL ("expected 'c' after 'p '");
  ch = NEXT ();
  if (ch != 'n')
    NONL ("expected 'n' after 'p c'");
  ch = NEXT ();
  if (ch != 'f')
    NONL ("expected 'n' after 'p cn'");
  ch = NEXT ();
  if (ch != ' ')
    NONL ("expected space after 'p cnf'");
  ch = NEXT ();
  if (strict != PEDANTIC_PARSING)
    {
      while (ch == ' ' || ch == '\t')
	ch = NEXT ();
    }
  if (!ISDIGIT (ch))
    NONL ("expected digit after 'p cnf '");
  int variables = ch - '0';
  while (ISDIGIT (ch = NEXT ()))
    {
      if (EXTERNAL_MAX_VAR / 10 < variables)
	return "maximum variable too large";
      variables *= 10;
      const int digit = ch - '0';
      if (EXTERNAL_MAX_VAR - digit < variables)
	return "maximum variable too large";
      variables += digit;
    }
  if (ch == EOF)
    return "unexpected end-of-file while parsing maximum variable";
  if (ch == '\r')
    {
      ch = NEXT ();
      if (ch != '\n')
	return "expected new-line after carriage-return";
    }
  if (ch == '\n')
    NONL ("unexpected new-line after maximum variable");
  if (ch != ' ')
    return "expected space after maximum variable";
  ch = NEXT ();
  if (strict != PEDANTIC_PARSING)
    {
      while (ch == ' ' || ch == '\t')
	ch = NEXT ();
    }
  if (!ISDIGIT (ch))
    return "expected number of clauses after maximum variable";
  uint64_t clauses = ch - '0';
  while (ISDIGIT (ch = NEXT ()))
    {
      if (UINT64_MAX / 10 < clauses)
	return "number of clauses too large";
      clauses *= 10;
      const int digit = ch - '0';
      if (UINT64_MAX - digit < clauses)
	return "number of clauses too large";
      clauses += digit;
    }
  if (ch == EOF)
    return "unexpected end-of-file while parsing number of clauses";
  if (strict != PEDANTIC_PARSING)
    {
      while (ch == ' ' || ch == '\t')
	ch = NEXT ();
    }
  if (ch == '\r')
    {
      ch = NEXT ();
      if (ch != '\n')
	return "expected new-line after carriage-return";
    }
  if (ch == EOF)
    return "unexpected end-of-file after parsing number of clauses";
  if (ch != '\n')
    return "expected new-line after parsing number of clauses";
  kissat_message (solver,
		  "parsed 'p cnf %d %" PRIu64 "' header", variables, clauses);
  *max_var_ptr = variables;
  kissat_reserve (solver, variables);
  uint64_t parsed = 0;
  int lit = 0;
  for (;;)
    {
      ch = NEXT ();
      if (ch == ' ')
	continue;
      if (ch == '\t')
	continue;
      if (ch == '\n')
	continue;
      if (ch == '\r')
	{
	  ch = NEXT ();
	  if (ch != '\n')
	    return "expected new-line after carriage-return";
	  continue;
	}
      if (ch == 'c')
	{
	  while ((ch = NEXT ()) != '\n')
	    if (ch == EOF)
	      {
		if (strict != PEDANTIC_PARSING)
		  break;
		return "unexpected end-of-file in comment after header";
	      }
	  if (ch == EOF)
	    break;
	  continue;
	}
      if (ch == EOF)
	break;
      int sign;
      if (ch == '-')
	{
	  ch = NEXT ();
	  if (ch == EOF)
	    return "unexpected end-of-file after '-'";
	  if (ch == '\n')
	    NONL ("unexpected new-line after '-'");
	  if (!ISDIGIT (ch))
	    return "expected digit after '-'";
	  if (ch == '0')
	    return "expected non-zero digit after '-'";
	  sign = -1;
	}
      else if (!ISDIGIT (ch))
	return "expected digit or '-'";
      else
	sign = 1;
      assert (ISDIGIT (ch));
      int idx = ch - '0';
      while (ISDIGIT (ch = NEXT ()))
	{
	  if (EXTERNAL_MAX_VAR / 10 < idx)
	    return "variable index too large";
	  idx *= 10;
	  const int digit = ch - '0';
	  if (EXTERNAL_MAX_VAR - digit < idx)
	    return "variable index too large";
	  idx += digit;
	}
      if (ch == EOF)
	{
	  if (strict == PEDANTIC_PARSING)
	    {
	      if (idx)
		return "unexpected end-of-file after literal";
	      else
		return "unexpected end-of-file after trailing zero";
	    }
	}
      else if (ch == '\r')
	{
	  ch = NEXT ();
	  if (ch != '\n')
	    return "expected new-line after carriage-return";
	}
      else if (ch == 'c')
	{
	  while ((ch = NEXT ()) != '\n')
	    if (ch == EOF)
	      {
		if (strict != PEDANTIC_PARSING)
		  break;
		return "unexpected end-of-file in comment after literal";
	      }
	}
      else if (ch != ' ' && ch != '\t' && ch != '\n')
	return "expected white space after literal";
      if (strict != RELAXED_PARSING && idx > variables)
	NONL ("maximum variable index exceeded " TRY_RELAXED_PARSING);
      if (idx)
	{
	  assert (sign == 1 || sign == -1);
	  assert (idx != INT_MIN);
	  lit = sign * idx;
	}
      else
	{
	  if (strict != RELAXED_PARSING && parsed == clauses)
	    return "too many clauses " TRY_RELAXED_PARSING;
	  parsed++;
	  lit = 0;
	}
      kissat_add (solver, lit);
    }
  if (lit)
    return "trailing zero missing";
  if (strict != RELAXED_PARSING && parsed < clauses)
    {
      if (parsed + 1 == clauses)
	return "one clause missing " TRY_RELAXED_PARSING;
      return "more than one clause missing " TRY_RELAXED_PARSING;
    }

  *lineno_ptr = lineno;

  return 0;
}

const char *
kissat_parse_dimacs (kissat * solver,
		     strictness strict,
		     file * file, uint64_t * lineno_ptr, int *max_var_ptr)
{
  START (parse);
  const char *res;
  res = parse_dimacs (solver, file, strict, lineno_ptr, max_var_ptr);
  if (!solver->inconsistent)
    kissat_defrag_watches (solver);
  STOP (parse);
  return res;
}
