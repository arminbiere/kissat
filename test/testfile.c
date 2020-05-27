#include "test.h"

#include "../src/file.h"

#include <inttypes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static bool
can_open_dev_null (void)
{
  FILE *file = fopen ("/dev/null", "r");
  if (!file)
    return false;
  fclose (file);
  return true;
}

static bool
etc_shadow_exists (void)
{
  struct stat buf;
  return !stat ("/etc/shadow", &buf);
}

static void
test_file_basic (void)
{
  assert (!kissat_file_readable (0));
  assert (!kissat_file_writable (0));
  file file;
  kissat_read_already_open_file (&file, stdin, "<stdin>");
  kissat_close_file (&file);
  kissat_write_already_open_file (&file, stdout, "<stdout>");
  kissat_close_file (&file);
  assert (!kissat_open_to_write_file (&file, "/root/directory/not-writable"));
  assert (!kissat_file_size ("/root/directory/not-writable"));
}

static void
test_file_readable (void)
{
#define READABLE(EXPECTED,PATH) \
do { \
  bool RES = kissat_file_readable (PATH); \
  if (RES && EXPECTED) \
    printf ("file '%s' determined to be readable as expected\n", PATH); \
  else if (!RES && ~EXPECTED) \
    printf ("file '%s' determined not to be readable as expected\n", PATH); \
  else if (RES && !EXPECTED) \
    FATAL ("file '%s' determined to be readable unexpectedly", PATH); \
  else if (!RES && EXPECTED) \
    FATAL ("file '%s' determined to not be readable unexpectedly", PATH); \
} while (0)
  assert (!kissat_file_readable (0));
  READABLE (false, "non-existing-file");
  READABLE (true, "tissat");
  READABLE (true, ".");
  if (etc_shadow_exists ())
    READABLE (false, "/etc/shadow");
  if (can_open_dev_null ())
    READABLE (true, "/dev/null");
#undef EXISTS
}

static void
test_file_writable (void)
{
#define WRITABLE(EXPECTED,PATH) \
do { \
  bool RES = kissat_file_writable (PATH); \
  if (RES && EXPECTED) \
    printf ("file '%s' determined to be writable as expected\n", PATH); \
  else if (!RES && ~EXPECTED) \
    printf ("file '%s' determined not to be writable as expected\n", PATH); \
  else if (RES && !EXPECTED) \
    FATAL ("file '%s' determined to be writable unexpectedly", PATH); \
  else if (!RES && EXPECTED) \
    FATAL ("file '%s' determined not to be writable unexpectedly", PATH); \
} while (0)
  assert (!kissat_file_writable (0));
  WRITABLE (true, "../test/file/writable");
  if (can_open_dev_null ())
    WRITABLE (true, "/dev/null");
  WRITABLE (false, "");
  WRITABLE (true, "non-existing-file");
  WRITABLE (false, ".");
  WRITABLE (false, "/");
  WRITABLE (false, "tissat/test");
  WRITABLE (true, "../test/file/non-existing");
  WRITABLE (false, "/kissat-test-file-writable");
  WRITABLE (false, "non-existing-directory/file-in-non-existing-directory");
  WRITABLE (false, "/etc/passwd");
#undef WRITABLE
}

#ifdef _POSIX_C_SOURCE

static void
test_file_read_compressed (void)
{
  const size_t expected_bytes = kissat_file_size ("../test/file/0");
#define READ_COMPRESSED(EXPECTED,EXECUTABLE,PATH) \
do { \
  file file; \
  if (!kissat_find_executable (EXECUTABLE)) \
    { \
      printf ("skipping '%s': could not find executable '%s'\n", \
              EXECUTABLE, PATH); \
      break; \
    } \
  bool res = kissat_open_to_read_file (&file, PATH); \
  if (res && EXPECTED) \
    printf ("opened compressed '%s' for reading as expected\n", PATH); \
  else if (!res && !EXPECTED) \
    printf ("failed to open compressed '%s' for reading as expected\n", \
            PATH); \
  else if (!res && EXPECTED) \
    FATAL ("failed to open compressed '%s' for reading unexpectedly", \
            PATH); \
  else if (res && !EXPECTED) \
    FATAL ("opened compressed '%s' for reading unexpectedly", PATH); \
  if (!res) \
    break; \
  int ch; \
  while ((ch = kissat_getc (&file)) != EOF) \
    ; \
  printf ("closing '%s' after reading '%" PRIu64 "' bytes\n", \
          PATH, file.bytes); \
  kissat_close_file (&file); \
  if (file.bytes != expected_bytes) \
    FATAL ("read '%" PRIu64 "' bytes but expected '%zu'", \
           file.bytes, expected_bytes); \
} while (0)
  READ_COMPRESSED (true, "bzip2", "../test/file/1.bz2");
  READ_COMPRESSED (true, "gzip", "../test/file/2.gz");
  READ_COMPRESSED (true, "lzma", "../test/file/3.lzma");
  READ_COMPRESSED (true, "7z", "../test/file/4.7z");
  READ_COMPRESSED (true, "xz", "../test/file/5.xz");
  READ_COMPRESSED (false, "bzip2", "../test/file/non-existing.bz2");
  READ_COMPRESSED (false, "gzip", "../test/file/non-existing.gz");
  READ_COMPRESSED (false, "lzma", "../test/file/non-existing.lzma");
  READ_COMPRESSED (false, "7z", "../test/file/non-existing.7z");
  READ_COMPRESSED (false, "xz", "../test/file/non-existing.xz");
  READ_COMPRESSED (true, "bzip2", "../test/file/uncompressed.bz2");
  READ_COMPRESSED (true, "gzip", "../test/file/uncompressed.gz");
  READ_COMPRESSED (true, "lzma", "../test/file/uncompressed.lzma");
  READ_COMPRESSED (true, "7z", "../test/file/uncompressed.7z");
  READ_COMPRESSED (true, "xz", "../test/file/uncompressed.xz");
}

#endif

static void
test_file_read_uncompressed (void)
{
  const size_t expected_bytes = kissat_file_size ("../test/file/0");
#define READ_UNCOMPRESSED(EXPECTED,PATH) \
do { \
  file file; \
  bool res = kissat_open_to_read_file (&file, PATH); \
  if (res && EXPECTED) \
    printf ("opened uncompressed '%s' for reading as expected\n", PATH); \
  else if (!res && !EXPECTED) \
    printf ("failed to open uncompressed '%s' for reading as expected\n", \
            PATH); \
  else if (!res && EXPECTED) \
    FATAL ("failed to open uncompressed '%s' for reading unexpectedly", \
            PATH); \
  else if (res && !EXPECTED) \
    FATAL ("opened uncompressed '%s' for reading unexpectedly", PATH); \
  if (!res) \
    break; \
  int ch; \
  while ((ch = kissat_getc (&file)) != EOF) \
    ; \
  printf ("closing '%s' after reading '%" PRIu64 "' bytes\n", \
          PATH, file.bytes); \
  kissat_close_file (&file); \
  if (file.bytes != expected_bytes) \
    FATAL ("read '%" PRIu64 "' bytes but expected '%zu'", \
           file.bytes, expected_bytes); \
} while (0)
  READ_UNCOMPRESSED (true, "../test/file/0");
  READ_UNCOMPRESSED (false, "../test/file/non-existing");
  READ_UNCOMPRESSED (true, "../test/file/uncompressed.bz2");
  READ_UNCOMPRESSED (true, "../test/file/uncompressed.gz");
  READ_UNCOMPRESSED (true, "../test/file/uncompressed.lzma");
  READ_UNCOMPRESSED (true, "../test/file/uncompressed.7z");
  READ_UNCOMPRESSED (true, "../test/file/uncompressed.xz");
}

#ifdef _POSIX_C_SOURCE

static void
test_file_write_and_read_compressed (void)
{
#define WRITE_AND_READ_COMPRESSED(EXECUTABLE,SUFFIX) \
do { \
  if (!kissat_find_executable (EXECUTABLE)) \
    printf ("not writing and reading compressed '%s' file " \
            "(could not find '%s' executable)", SUFFIX, EXECUTABLE); \
  else \
    { \
      printf ("found '%s' executable in path\n", EXECUTABLE); \
      file file; \
      const char * path = "42" SUFFIX; \
      printf ("writing single '42' line to compressed '%s'\n", path); \
      if (!kissat_open_to_write_file (&file, path)) \
	FATAL ("failed to write compressed '%s'", path); \
      else \
	{ \
	  kissat_putc (&file, '4'); \
	  kissat_putc (&file, '2'); \
	  kissat_putc (&file, '\n'); \
	  kissat_close_file (&file); \
	  printf ("reading single '42' line from compressed '%s'\n", path); \
	  if (!kissat_open_to_read_file (&file, path)) \
	    FATAL ("failed to read compressed '%s'", path); \
	  int chars = 0; \
	  if ((++chars && kissat_getc (&file) != '4') || \
	      (++chars && kissat_getc (&file) != '2') || \
	      (++chars && kissat_getc (&file) != '\n') || \
	      (++chars && kissat_getc (&file) != EOF)) \
	    FATAL ("failed to read single '42' line from '%s' " \
	           "(character %d wrong)", path, chars); \
	} \
    } \
} while (0)
  WRITE_AND_READ_COMPRESSED ("7z", ".7z");
  WRITE_AND_READ_COMPRESSED ("bzip2", ".bz2");
  WRITE_AND_READ_COMPRESSED ("gzip", ".gz");
  WRITE_AND_READ_COMPRESSED ("lzma", ".lzma");
  WRITE_AND_READ_COMPRESSED ("xz", ".xz");
}

#endif

void
tissat_schedule_file (void)
{
  SCHEDULE_FUNCTION (test_file_basic);
  SCHEDULE_FUNCTION (test_file_readable);
  if (tissat_found_test_directory)
    SCHEDULE_FUNCTION (test_file_writable);
  if (tissat_found_test_directory)
    SCHEDULE_FUNCTION (test_file_read_uncompressed);
#ifdef _POSIX_C_SOURCE
  SCHEDULE_FUNCTION (test_file_write_and_read_compressed);
  if (tissat_found_test_directory)
    SCHEDULE_FUNCTION (test_file_read_compressed);
#endif
}
