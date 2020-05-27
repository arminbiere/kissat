#ifndef _reference_h_INCLUDED
#define _reference_h_INCLUDED

#include "stack.h"

typedef unsigned reference;

#define LD_MAX_REF 31
#define MAX_REF (((reference)1 << LD_MAX_REF)-1)

#define INVALID_REF UINT_MAX

// *INDENT-OFF*
typedef STACK (reference) references;
// *INDENT-ON*

#endif
