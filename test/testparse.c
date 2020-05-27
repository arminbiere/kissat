#include "test.h"

#include "../src/file.h"
#include "../src/parse.h"

#include <inttypes.h>

static bool
test_parse (bool expect_parse_error, unsigned strict, const char *path)
{
  const char *type;
  switch (strict)
    {
    default:
    case RELAXED_PARSING:
      type = "relaxed";
      break;
    case NORMAL_PARSING:
      type = "normal";
      break;
    case PEDANTIC_PARSING:
      type = "pedantic";
      break;
    }
  tissat_verbose ("Parsing %svalid '%s' in '%s' mode.",
		  expect_parse_error ? "in" : "", path, type);
  kissat *solver = kissat_init ();
  tissat_init_solver (solver);
  file file;
  if (!kissat_open_to_read_file (&file, path))
    FATAL ("could not open '%s' for reading", path);
  uint64_t lineno;
  int max_var;
  const char *error =
    kissat_parse_dimacs (solver, strict, &file, &lineno, &max_var);
  if (expect_parse_error)
    {
      if (!error)
	FATAL ("%s parsing '%s' succeeded unexpectedly", type, path);
      tissat_verbose ("%s:%" PRIu64 ": %s", path, lineno, error);
    }
  else if (!expect_parse_error && error)
    {
      FATAL ("%s parsing failed unexpectedly: %s:%" PRIu64 ": %s",
	     type, path, lineno, error);
      tissat_verbose ("found maximum variable '%d' in '%s'", max_var, path);
    }
  kissat_close_file (&file);
  kissat_release (solver);
  return false;
}

static void
test_parse_errors (void)
{
#define PARSE(STRICT,NAME) \
do {  \
  const char * path = "../test/parse/" #NAME; \
  bool expect_error; \
  if (STRICT == RELAXED_PARSING) \
    expect_error = true;  \
  else if (STRICT == NORMAL_PARSING) \
    expect_error = (strict == NORMAL_PARSING || strict == PEDANTIC_PARSING); \
  else \
    assert (STRICT == PEDANTIC_PARSING), \
    expect_error = (strict == PEDANTIC_PARSING); \
  test_parse (expect_error, strict, path); \
} while (0)
  for (strictness strict = RELAXED_PARSING;
       strict <= PEDANTIC_PARSING; strict++)
    {
      PARSE (0, emptyfile);
      PARSE (0, onlycomments);
      PARSE (0, eofheadercomment);
      PARSE (0, eofinheaderafterc);
      PARSE (0, eofbeforeheader);
      PARSE (0, eofinheaderafterc);
      PARSE (0, nonlaftercrafterheadliner);
      PARSE (0, onlycr);
      PARSE (0, emptyline);
      PARSE (2, emptyheaderline);
      PARSE (2, emptyheadercrnl);
      PARSE (0, embeddedcrmissingnl);
      PARSE (0, nonlafterinvalidembeddedoption);
      PARSE (0, nocnorp);
      PARSE (0, nospaceafterp);
      PARSE (0, nlafterp);
      PARSE (0, nocafterpspace);
      PARSE (0, nonafterc);
      PARSE (0, nofaftern);
      PARSE (0, nospaceafterf);
      PARSE (0, nodigitafterpcnfspace);
      PARSE (0, toolargevars1);
      PARSE (0, toolargevars2);
      PARSE (0, eofinmaxvar);
      PARSE (0, onlycraftervars);
      PARSE (0, nlaftervars);
      PARSE (0, nospaceaftervars);
      PARSE (0, nodigitaftervarsnspace);
      PARSE (0, toolargeclauses1);
      PARSE (0, toolargeclauses2);
      PARSE (0, nonlaftercrafterheaderline);
      PARSE (0, eofinclauses);
      PARSE (0, nonlafterheaderline);
      PARSE (0, othercharafterheaderline);
      PARSE (0, nonlaftercrinbody);
      PARSE (2, eofbodycomment);
      PARSE (0, eofafterlit);
      PARSE (2, eofafterzero);
      PARSE (0, signeof);
      PARSE (0, nlaftersign);
      PARSE (0, nodigitaftersign);
      PARSE (0, zeroaftersign);
      PARSE (0, anainsteadoflit);
      PARSE (1, toomanyclauses);
      PARSE (0, varidxtoolarge1);
      PARSE (0, varidxtoolarge2);
      PARSE (0, nonlaftercrafterlit);
      PARSE (0, nowsafterlit);
      PARSE (1, varidxexceeded);
      PARSE (0, notrailingzero);
      PARSE (1, oneclausemissing);
      PARSE (1, twoclausesmissing);
      PARSE (2, tabs);
      PARSE (2, headerspaces);
      PARSE (2, eofincommmentafterliteral);
    }
#undef PARSE
}

static void
test_parse_coverage (void)
{
#define PARSE(STRICT,NAME) \
do {  \
  if (!strict && STRICT) \
    break; \
  if (strict == 1 && STRICT == 2) \
    break; \
  const char * path = "../test/parse/" #NAME; \
  test_parse (false, strict, path); \
} while (0)
  for (unsigned strict = 0; strict <= 2; strict++)
    {
      PARSE (0, crnl);
      PARSE (0, signedunit);
      PARSE (0, comments);
      PARSE (0, invalidembeddedoption);
      PARSE (0, embeddedoptiocrnl);
      PARSE (0, embeddedoptioneqmissing);
      PARSE (0, embeddedoptionametoolong);
      PARSE (0, embeddednegative);
      PARSE (0, embeddedcoverage);
      PARSE (0, crnlfile);
    }
#undef PARSE
}

void
tissat_schedule_parse (void)
{
  if (tissat_found_test_directory)
    SCHEDULE_FUNCTION (test_parse_errors);
  if (tissat_found_test_directory)
    SCHEDULE_FUNCTION (test_parse_coverage);
}
