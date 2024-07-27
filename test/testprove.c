#ifndef NPROOFS

#include "../src/file.h"

#include "test.h"
#include "testcnfs.h"

#ifdef KISSAT_COMPRESSED

#define MAX_COMPRESSED 10

static const char *compressions[MAX_COMPRESSED];
static unsigned size_compressions;

#define NEW_COMPRESSION(NAME, SUFFIX) \
  do { \
    if (!tissat_found_##NAME) \
      break; \
    assert (size_compressions < MAX_COMPRESSED); \
    compressions[size_compressions++] = SUFFIX; \
  } while (0)

static void init_compression (void) {
  // clang-format off
  NEW_COMPRESSION (bzip2, ".bz2");
  NEW_COMPRESSION (gzip, ".gz");
  NEW_COMPRESSION (lzma, ".lzma");
  NEW_COMPRESSION (7z, ".7z");
  NEW_COMPRESSION (xz, ".xz");
  // clang-format on
}

static unsigned compressed;

#endif

static unsigned scheduled;

static void schedule_prove_job_with_option (int expected, const char *opt,
                                            const char *cnf,
                                            const char *name) {
  char cmd[256];
  if (!kissat_file_readable (cnf)) {
    tissat_warning ("Skipping unreadable '%s'", cnf);
    return;
  }

  bool drat_trim = false;
  bool dpr_trim = false;
  bool drabt = false;

  if (!strcmp (name, "false")) {
    drabt = tissat_found_drabt;
  } else if (tissat_big) {
    drabt = tissat_found_drabt;
    drat_trim = tissat_found_drat_trim;
    dpr_trim = tissat_found_dpr_trim;
  } else {
    int drabt_pos = -1;
    int drat_trim_pos = -1;
    int dpr_trim_pos = -1;
    int pos = 0;
    if (tissat_found_drabt)
      drabt_pos = pos++;
    if (tissat_found_drat_trim)
      drat_trim_pos = pos++;
    if (tissat_found_dpr_trim)
      dpr_trim_pos = pos++;
    if (pos) {
      pos = scheduled % pos;
      drabt = (pos == drabt_pos);
      drat_trim = (pos == drat_trim_pos);
      dpr_trim = (pos == dpr_trim_pos);
    }
  }

  const char *suffix = "";

#ifdef KISSAT_COMPRESSED
  bool compress;
  if (!size_compressions)
    compress = false;
  else if (drat_trim)
    compress = false;
  else if (dpr_trim)
    compress = false;
  else if (drabt)
    compress = (scheduled & 2);
  else
    compress = true;

  if (compress)
    suffix = compressions[compressed++ % size_compressions];
#endif
  char proof[96];
  sprintf (proof, "%s.proof%u%s", name, scheduled, suffix);

  const char *binary = (scheduled % 5) ? "" : "--no-binary ";
  sprintf (cmd, "%s%s%s %s", opt, binary, cnf, proof);
  tissat_job *job = tissat_schedule_application (expected, cmd);
  scheduled++;

  if (drabt) {
    if (expected == 10)
      sprintf (cmd, "drabt -S %s %s", cnf, proof);
    else
      sprintf (cmd, "drabt %s %s", cnf, proof);
    assert (strlen (cmd) < sizeof cmd);

    tissat_schedule_command (0, cmd, job);
  }

  if (drat_trim) {
    if (expected == 10) {
#if 0
	  sprintf (cmd, "drat-trim %s %s -S", cnf, proof);
	  assert (strlen (cmd) < sizeof cmd);
	  tissat_schedule_command (0, cmd, job);
#endif
    } else {
      sprintf (cmd, "drat-trim %s %s", cnf, proof);
      assert (strlen (cmd) < sizeof cmd);
      tissat_schedule_command (0, cmd, job);
    }
  }

  if (dpr_trim) {
    if (expected == 10) {
#if 0
	  sprintf (cmd, "dpr-trim %s %s -S", cnf, proof);
	  assert (strlen (cmd) < sizeof cmd);
	  tissat_schedule_command (0, cmd, job);
#endif
    } else {
      sprintf (cmd, "dpr-trim %s %s", cnf, proof);
      assert (strlen (cmd) < sizeof cmd);
      tissat_schedule_command (0, cmd, job);
    }
  }
}

static void schedule_prove_job (int expected, const char *cnf,
                                const char *name) {
  if (tissat_big) {
    for (all_tissat_options (opt))
      schedule_prove_job_with_option (expected, opt, cnf, name);
  } else {
    const char *opt = tissat_next_option (scheduled);
    schedule_prove_job_with_option (expected, opt, cnf, name);
  }
}

void tissat_schedule_prove (void) {
#ifdef KISSAT_COMPRESSED
  init_compression ();
#endif
#define CNF(EXPECTED, NAME, BIG) \
  if (!BIG || tissat_big) \
    schedule_prove_job (EXPECTED, "../test/cnf/" #NAME ".cnf", #NAME);
  CNFS
#undef CNF
}

#else
int tissat_prove_do_avoid_warning;
#endif
