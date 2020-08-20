#ifndef _frames_h_INCLUDED
#define _frames_h_INCLUDED

#include "literal.h"
#include "stack.h"

#include <stdbool.h>

typedef struct frame frame;

#define LD_MAX_TRAIL LD_MAX_VAR
#define MAX_TRAIL ((1u<<LD_MAX_TRAIL)-1)
#define INVALID_TRAIL UINT_MAX

struct frame
{
  unsigned decision;
  unsigned trail:LD_MAX_TRAIL;
  unsigned used:2;
  bool promote:1;
};

// *INDENT-OFF*

typedef STACK (frame) frames;

// *INDENT-ON*

struct kissat;

void kissat_push_frame (struct kissat *, unsigned decision);

#define FRAME(LEVEL) \
  (PEEK_STACK (solver->frames, (LEVEL)))

#endif
