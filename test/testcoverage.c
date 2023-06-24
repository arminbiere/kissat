#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>

#include "test.h"

static bool is_cover_file_name (const char *name) {
#define CHAR(CH) (*p++ == CH)
#define DIGIT() (isdigit ((int) *p++))
  const char *p = name;
  // clang-format off
  return 
    CHAR ('c') && CHAR ('o') && CHAR ('v') && CHAR ('e') && CHAR ('r') &&
    DIGIT () && DIGIT () && DIGIT () && DIGIT () &&
    CHAR ('.') && CHAR ('c') && CHAR ('n') && CHAR ('f');
// clang-format on
#undef CHAR
#undef DIGIT
}

static void schedule_cover_file (const char *dir, const char *name) {
  char path[512];
  assert (dir[0]);
  assert (dir[strlen (dir) - 1] == '/');
  assert (strlen (dir) + strlen (name) + 1 < sizeof path);
  sprintf (path, "%s%s", dir, name);
  FILE *file = fopen (path, "r");
  if (!file)
    FATAL ("could not read '%s'", path);
  int status;
  if (fscanf (file, "c status %d\n", &status) != 1)
    FATAL ("parse error at line 1 in '%s': expected 'c status <status>'",
           path);
  fclose (file);
  tissat_schedule_application (status, path);
}

#define MAX_COVER_FILES (1 << 16)

static char *cover_files[MAX_COVER_FILES];
static size_t size_cover_files;

static void push_cover_file (const char *name) {
  assert (size_cover_files < MAX_COVER_FILES);
  char *tmp = malloc (strlen (name) + 1);
  cover_files[size_cover_files++] = strcpy (tmp, name);
}

static int cmp (const void *p, const void *q) {
  return strcmp (*(char **) p, *(char **) q);
}

static void sort_cover_files (void) {
  qsort (cover_files, size_cover_files, sizeof (char *), cmp);
}

void tissat_schedule_coverage (void) {
  if (!tissat_found_test_directory)
    return;
  const char *path = "../test/cover/";
  DIR *dir = opendir (path);
  if (!dir)
    FATAL ("could not open directory '%s' "
           "(even though '../test/' claimed to exist)",
           path);
  struct dirent *entry;
  const char *name;
  while ((entry = readdir (dir)))
    if (is_cover_file_name ((name = entry->d_name)))
      push_cover_file (name);
  closedir (dir);
  sort_cover_files ();
  for (size_t i = 0; i < size_cover_files; i++)
    schedule_cover_file (path, cover_files[i]);
  for (size_t i = 0; i < size_cover_files; i++)
    free (cover_files[i]);
}
