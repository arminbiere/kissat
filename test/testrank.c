#include "../src/allocate.h"
#include "../src/rank.h"
#include "../src/stack.h"

#include "test.h"

static unsigned rank_unsigned (unsigned a) { return a; }

static void test_rank_unsigneds (void) {
#define M 2
  srand (42);
  printf ("running %d rounds\n", M);
  for (unsigned i = 0; i < M; i++) {
#define N 80000
    DECLARE_AND_INIT_SOLVER (solver);
    unsigned found[N];
    memset (found, 0, sizeof found);
    unsigned A[N];
    printf ("generating and ranking %d random numbers\n", N);
    for (unsigned j = 0; j < N; j++)
      A[j] = rand () % N;
    for (unsigned j = 0; j < N; j++)
      found[A[j]]++;
#if 0
      printf ("before radix sorting");
      for (unsigned j = 0; j < N; j++)
	printf (" %u", A[j]);
      printf ("\n");
      fflush (stdout);
#endif
    RADIX_SORT (unsigned, unsigned, N, A, rank_unsigned);
#if 0
      printf ("after radix sorting");
      for (unsigned j = 0; j < N; j++)
	printf (" %u", A[j]);
      printf ("\n");
      fflush (stdout);
#endif
    for (unsigned j = 1; j < N; j++)
      assert (A[j - 1] <= A[j]);
    for (unsigned j = 0; j < N; j++) {
      assert (A[j] < N);
      assert (found[A[j]] > 0);
      found[A[j]]--;
    }
    for (unsigned j = 0; j < N; j++)
      assert (!found[j]);
#ifndef QUIET
    RELEASE_STACK (solver->profiles.stack);
#endif
#ifdef METRICS
    assert (!solver->statistics.allocated_current);
#endif
#undef N
  }
#undef M
}

#include <inttypes.h>
#include <string.h>

static uint64_t rank_string (const char *str) {
  unsigned char ch;
  uint64_t res = 0;
  unsigned i = 64;
  for (const char *p = str; (ch = *p); p++) {
    assert (i);
    i -= 8;
    uint64_t tmp = ch;
    res |= tmp << i;
  }
  return res;
}

static void test_rank_strings (void) {
#define N 10
  DECLARE_AND_INIT_SOLVER (solver);
  char S[N][9];
  memset (S, 0, sizeof S);
  char *A[N];
  for (unsigned i = 0; i < N; i++)
    A[i] = S[i];
  srand (42);
  for (unsigned i = 0; i < N; i++) {
    unsigned len = (rand () % 8) + 1;
    for (unsigned j = 0; j < len; j++)
      S[i][j] = 'a' + (rand () % 26);
    S[i][len] = 0;
  }
  printf ("before radix sorting:\n\n");
  for (unsigned i = 0; i < N; i++)
    printf ("A[%u] %s\n", i, A[i]);
  fflush (stdout);
  RADIX_SORT (char *, uint64_t, N, A, rank_string);
  printf ("\nafter radix sorting:\n\n");
  for (unsigned i = 0; i < N; i++)
    printf ("A[%u] %s\n", i, A[i]);
  fflush (stdout);
  for (unsigned i = 1; i < N; i++)
    assert (strcmp (A[i - 1], A[i]) <= 0);
#ifndef QUIET
  RELEASE_STACK (solver->profiles.stack);
#endif
#ifdef METRICS
  assert (!solver->statistics.allocated_current);
#endif
}

void tissat_schedule_rank (void) {
  SCHEDULE_FUNCTION (test_rank_unsigneds);
  SCHEDULE_FUNCTION (test_rank_strings);
}
