#include "test.h"

static void test_options_parse_value (void) {
  int value;

#define CHECKVAL(STR, VAL) \
  do { \
    assert (kissat_parse_option_value (STR, &value)); \
    if (value != VAL) \
      FATAL ("parsing '%s' gives '%d' and not '%s' (%d)", STR, value, \
             #VAL, (int) (VAL)); \
    else \
      printf ("checked parsing value string '%s' as '%s'\n", STR, #VAL); \
  } while (0)

  CHECKVAL ("0", 0);
  CHECKVAL ("1", 1);
  CHECKVAL ("-1", -1);
  CHECKVAL ("2147483647", INT_MAX);
  CHECKVAL ("-2147483648", INT_MIN);
  CHECKVAL ("0e0", 0);
  CHECKVAL ("0e123123123", 0);
  CHECKVAL ("1e0", 1e0);
  CHECKVAL ("-1e0", -1);
  CHECKVAL ("2e0", 2e0);
  CHECKVAL ("3e1", 3e1);
  CHECKVAL ("4e8", 4e8);
  CHECKVAL ("2e9", 2e9);
  CHECKVAL ("-2e9", -2e9);
  CHECKVAL ("0^123123", 0);
  CHECKVAL ("-1^123123", -1);
  CHECKVAL ("2^5", 32);
  CHECKVAL ("-2^31", INT_MIN);
  CHECKVAL ("2^30", (1 << 30));
  CHECKVAL ("3^3", 27);
  CHECKVAL ("5^0", 1);

#undef CHECKVAL

#define FAILVAL(STR) \
  do { \
    assert (!kissat_parse_option_value (STR, &value)); \
    printf ("checked parsing value string '%s' failed as expected\n", \
            STR); \
  } while (0)

  FAILVAL ("");
  FAILVAL ("0oops");
  FAILVAL ("00");
  FAILVAL ("0^0");
  FAILVAL ("2147483648");
  FAILVAL ("2147483649");
  FAILVAL ("3000000000");
  FAILVAL ("1e");
  FAILVAL ("1e10");
  FAILVAL ("0e11111oops");
  FAILVAL ("2^");
  FAILVAL ("2^00");
  FAILVAL ("2^33");
  FAILVAL ("2^123");
  FAILVAL ("1^11111oops");
  FAILVAL ("-3e123123");
  FAILVAL ("3e9");
  FAILVAL ("-3e9");
  FAILVAL ("1e1oops");
  FAILVAL ("2^1oops");

#undef FAILVAL
}

static void test_options_parse_name (void) {
  const char *valstr;

#define CHECKNAME(STR, NAME, EXPECTED) \
  do { \
    valstr = kissat_parse_option_name (STR, NAME); \
    if (!valstr) \
      FATAL ("kissat_parse_option_name (\"" STR "\", \"" NAME \
             "\") returns 0"); \
    if (strcmp (valstr, EXPECTED)) \
      FATAL ("kissat_parse_option_name (\"" STR "\", \"" NAME \
             "\") != \"" EXPECTED "\""); \
    printf ( \
        "checked parsing of option string '%s' name '%s' expected '%s'\n", \
        STR, NAME, EXPECTED); \
  } while (0)

  CHECKNAME ("--=", "", "");
  CHECKNAME ("--conflicts=", "conflicts", "");
  CHECKNAME ("--conflicts=0", "conflicts", "0");
  CHECKNAME ("--conflicts=1000", "conflicts", "1000");
  CHECKNAME ("--decisions=2^20", "decisions", "2^20");

#undef CHECKNAME

#define FAILNAME(STR, NAME) \
  do { \
    valstr = kissat_parse_option_name (STR, NAME); \
    if (valstr) \
      FATAL ("kissat_parse_option_name (\"" STR "\", \"" NAME \
             "\") succeeded"); \
    printf ("checked parsing of option string '%s' name '%s' failed as " \
            "expected\n", \
            STR, NAME); \
  } while (0)

  FAILNAME ("--conflicts=1", "decisions");
  FAILNAME ("--conflicts=1", "");
  FAILNAME ("--a=1", "b");
  FAILNAME ("--a=1", "aa");
  FAILNAME ("--aa=1", "a");
  FAILNAME ("--=1", "conflicts");

#undef FAILNAME
}

#ifndef NOPTIONS

static void test_options_basic (void) {
  options options;
  kissat_init_options (&options);
  const opt *opt = kissat_options_has ("eliminate");
  assert (opt);
  assert (opt->low == 0);
  assert (opt->high == 1);
  kissat_options_set_opt (&options, opt, 0);
  assert (options.eliminate == 0);
  kissat_options_set_opt (&options, opt, 1);
  assert (options.eliminate == 1);
  kissat_options_set_opt (&options, opt, -1);
  assert (options.eliminate == 0);
  kissat_options_set_opt (&options, opt, 2);
  assert (options.eliminate == 1);
  kissat_options_set_opt (&options, opt, INT_MIN);
  assert (options.eliminate == 0);
  kissat_options_set_opt (&options, opt, INT_MAX);
  assert (options.eliminate == 1);

  assert (!kissat_options_get (&options, "nonexistingoption"));
}

static void test_options_parse_arg (void) {
  char buffer[kissat_options_max_name_buffer_size];
  int value;

#define CHECKARG(STR, NAME, VAL) \
  do { \
    assert (kissat_options_parse_arg (STR, buffer, &value)); \
    if (strcmp (buffer, NAME)) \
      FATAL ("parsing '%s' yields name '%s' and not '%s'", STR, buffer, \
             NAME); \
    if (value != VAL) \
      FATAL ("parsing '%s' yields value '%d' and not '%s' (%d)", STR, \
             value, #VAL, (int) (VAL)); \
    printf ("checked parsing option string '%s' name '%s' value '%s'\n", \
            STR, NAME, #VAL); \
  } while (0)

  CHECKARG ("--reduce", "reduce", 1);
  CHECKARG ("--no-reduce", "reduce", 0);
  CHECKARG ("--reduce=true", "reduce", true);
  CHECKARG ("--reduce=false", "reduce", false);

#undef CHECKARG

#define FAILARG(STR) \
  do { \
    assert (!kissat_options_parse_arg (STR, buffer, &value)); \
    printf ("checked parsing option string '%s' failed as expected\n", \
            STR); \
  } while (0)

  FAILARG ("");
  FAILARG ("-");
  FAILARG ("--");
  FAILARG ("--none");
  FAILARG ("--no-ne");
  FAILARG ("--none=0");
  FAILARG ("--none=00");
  FAILARG ("-reduce");
  FAILARG ("--reduce=");
  FAILARG ("--reduce=a");
  FAILARG ("--no-reduce=a");
  FAILARG ("--waywaywaywaywaywaywaywaywaywaywaywaywaytoolong=0");
  FAILARG ("--reduce=00");
  FAILARG ("--reduce=2147483648");
  FAILARG ("--reduce=-1");
  FAILARG ("--reduce=2");

  // Force name too large for sure.
  //
  {
    char tmp[2 * kissat_options_max_name_buffer_size];
    const size_t len = sizeof tmp - 1;
    memset (tmp, 'a', len);
    tmp[0] = '-';
    tmp[1] = '-';
    tmp[len] = 0;
    FAILARG (tmp);
  }

#undef FAILARG
}

#endif

void tissat_schedule_options (void) {
  SCHEDULE_FUNCTION (test_options_parse_value);
  SCHEDULE_FUNCTION (test_options_parse_name);
#ifndef NOPTIONS
  SCHEDULE_FUNCTION (test_options_basic);
  SCHEDULE_FUNCTION (test_options_parse_arg);
#endif
}
