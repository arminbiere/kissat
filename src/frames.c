#include "allocate.h"
#include "internal.h"

void
kissat_push_frame (kissat * solver, unsigned decision)
{
  const size_t trail = SIZE_STACK (solver->trail);
  assert (trail <= MAX_TRAIL);
  frame frame;
  frame.decision = decision;
  frame.trail = trail;
  frame.promote = false;
  frame.used = 0;
  PUSH_STACK (solver->frames, frame);
}
