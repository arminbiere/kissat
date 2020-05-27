#include "test.h"

#include "../src/random.h"

static void
test_random_range (void)
{
  srand (42);
  generator random;
  const unsigned rounds = (tissat_big ? 100 : 10);
  for (unsigned seed = 0; seed < rounds; seed++)
    {
#define M 100
      random = seed;
      printf ("seed %u\n", seed);
      unsigned count[M];
      const unsigned L = rand () % 1000;
      const unsigned R = L + (rand () % M) + 1;
      const unsigned D = R - L;
      const unsigned N = 1000 * D;
      printf ("generating %u in the range [%u..%u) of size %u\n", N, L, R, D);
      const unsigned expected = N / D;
      assert (expected > 0);
      const double percent = 20;
      unsigned epsilon = percent / 100 * expected;
      if (!epsilon)
	epsilon = 1;
      assert (epsilon <= expected);
      printf ("expected %u epsilon %u = %.0f %% of %u (%u .. %u)\n",
	      expected, epsilon, percent, expected,
	      expected - epsilon, expected + epsilon);
      memset (count, 0, sizeof count);
      for (unsigned i = 0; i < N; i++)
	{
	  const unsigned picked = kissat_pick_random (&random, L, R);
	  assert (L <= picked), assert (picked < R);
	  count[picked - L]++;
	}
      unsigned min = UINT_MAX, max = 0;
      for (unsigned i = L; i < R; i++)
	{
	  const unsigned tmp = count[i - L];
	  max = MAX (tmp, max);
	  min = MIN (tmp, min);
	  printf ("count[%u] = %u\n", i, tmp);
	}
      printf ("min %u, max %u, range %.0f%%\n",
	      min, max, 100.0 * (max - min + 1) / expected);
      for (unsigned i = L; i < R; i++)
	if (count[i - L] < expected - epsilon)
	  FATAL ("count[%u] = %u but expected at least %u = %u - %u",
		 i, count[i - L], expected - epsilon, expected, epsilon);
	else if (count[i - L] > expected + epsilon)
	  FATAL ("count[%u] = %u but expected at most %u = %u + %u",
		 i, count[i - L], expected + epsilon, expected, epsilon);
    }
}

static void
test_random_bool (void)
{
  generator random;
  const unsigned rounds = (tissat_big ? 100 : 10);
  for (unsigned seed = 0; seed < rounds; seed++)
    {
      random = seed;
      printf ("seed %u\n", seed);
      unsigned count[2];
      const unsigned N = 10000;
      printf ("generating %u random Boolean numbers\n", N);
      const unsigned expected = N / 2;
      assert (expected > 0);
      const double percent = 10;
      unsigned epsilon = percent / 100 * expected;
      assert (epsilon);
      printf ("expected %u epsilon %u = %.0f %% of %u (%u .. %u)\n",
	      expected, epsilon, percent, expected,
	      expected - epsilon, expected + epsilon);
      memset (count, 0, sizeof count);
      for (unsigned i = 0; i < N; i++)
	{
	  const bool picked = kissat_pick_bool (&random);
	  count[picked]++;
	}
      for (unsigned i = 0; i < 2; i++)
	printf ("count[%u] = %u\n", i, count[i]);
      unsigned min = MIN (count[0], count[1]);
      unsigned max = MAX (count[0], count[1]);
      printf ("min %u, max %u, range %.0f%%\n",
	      min, max, 100.0 * (max - min + 1) / expected);
      for (unsigned i = 0; i < 2; i++)
	if (count[i] < expected - epsilon)
	  FATAL ("count[%u] = %u but expected at least %u = %u - %u",
		 i, count[i], expected - epsilon, expected, epsilon);
	else if (count[i] > expected + epsilon)
	  FATAL ("count[%u] = %u but expected at most %u = %u + %u",
		 i, count[i], expected + epsilon, expected, epsilon);
    }
}

void
tissat_schedule_random (void)
{
  SCHEDULE_FUNCTION (test_random_range);
  SCHEDULE_FUNCTION (test_random_bool);
}
