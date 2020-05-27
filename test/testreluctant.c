#include "test.h"

#include "../src/reluctant.h"

static void
test_reluctant (void)
{
  reluctant dummy_reluctant, *reluctant = &dummy_reluctant;
  memset (reluctant, 0, sizeof *reluctant);
#define LD_PERIOD (tissat_big ? 10 : 7)
#define LD_LIMIT (LD_PERIOD + 6)
#define LD_TICKS (LD_LIMIT + 6)
#define PERIOD (1u<<LD_PERIOD)
#define LIMIT (1u<<LD_LIMIT)
#define TICKS (1u<<LD_TICKS)
  printf ("period 2^%u = %u, limit 2^%u = %u, ticks 2^%u = %u\n",
	  LD_PERIOD, PERIOD, LD_LIMIT, LIMIT, LD_TICKS, TICKS);
  kissat_enable_reluctant (reluctant, PERIOD, LIMIT);
  assert (!kissat_reluctant_triggered (reluctant));
  unsigned triggered = 0, last = 0;
  for (unsigned ticks = 1; ticks <= TICKS; ticks++)
    {
      kissat_tick_reluctant (reluctant);
      if (!kissat_reluctant_triggered (reluctant))
	continue;
      triggered++;
      unsigned delta = ticks - last;
      printf ("triggered %u, tick %u, last %u, delta %u\n",
	      triggered, ticks, last, delta);
      last = ticks;
    }
}

void
tissat_schedule_reluctant (void)
{
  SCHEDULE_FUNCTION (test_reluctant);
}
