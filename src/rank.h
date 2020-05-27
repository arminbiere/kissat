#ifndef _rank_h_INCLUDED
#define _rank_h_INCLUDED

#include "allocate.h"

#include <string.h>

#ifdef NDEBUG
#define CHECK_RANKED(...) do { } while (0)
#else
#define CHECK_RANKED(N,A,RANK) \
do { \
  for (size_t I_CHECK_RANKED = 0; I_CHECK_RANKED < N-1; I_CHECK_RANKED++) \
    assert (RANK (A[I_CHECK_RANKED]) <= RANK (A[I_CHECK_RANKED + 1])); \
} while (0)
#endif

#define RADIX(LENGTH,VTYPE,RTYPE,N,V,RANK) \
do { \
  const size_t N_RADIX = (N); \
  if (N_RADIX <= 1) \
    break; \
  \
  START (radix); \
  \
  VTYPE * V_RADIX = (V); \
  \
  const size_t LENGTH_RADIX = (LENGTH); \
  const size_t WIDTH_RADIX = (1 << LENGTH_RADIX); \
  const RTYPE MASK_RADIX = WIDTH_RADIX - 1; \
  \
  size_t COUNT_RADIX[WIDTH_RADIX]; \
  \
  VTYPE * TMP_RADIX = 0; \
  const size_t BYTES_TMP_RADIX = N_RADIX * sizeof (VTYPE); \
  \
  VTYPE * A_RADIX = V_RADIX; \
  VTYPE * B_RADIX = 0; \
  VTYPE * C_RADIX = A_RADIX; \
  \
  RTYPE MLOWER_RADIX = 0; \
  RTYPE MUPPER_RADIX = MASK_RADIX; \
  \
  for (size_t I_RADIX = 0; \
       I_RADIX < 8 * sizeof (RTYPE); \
       I_RADIX += LENGTH_RADIX) \
    { \
      memset (COUNT_RADIX + MLOWER_RADIX, 0, \
              (MUPPER_RADIX - MLOWER_RADIX + 1) * sizeof *COUNT_RADIX); \
      \
      VTYPE * END_RADIX = C_RADIX + N_RADIX; \
      RTYPE UPPER_RADIX = 0; \
      RTYPE LOWER_RADIX = ~UPPER_RADIX; \
      \
      for (VTYPE * P_RADIX = C_RADIX; P_RADIX != END_RADIX; P_RADIX++) \
	{ \
	  RTYPE R_RADIX = RANK (*P_RADIX); \
	  RTYPE S_RADIX = R_RADIX >> I_RADIX; \
	  RTYPE M_RADIX = S_RADIX & MASK_RADIX; \
	  LOWER_RADIX &= S_RADIX; \
	  UPPER_RADIX |= S_RADIX; \
	  COUNT_RADIX[M_RADIX]++; \
	} \
      \
      if (LOWER_RADIX == UPPER_RADIX) \
	break; \
      \
      MLOWER_RADIX = LOWER_RADIX & MASK_RADIX; \
      MUPPER_RADIX = UPPER_RADIX & MASK_RADIX; \
      \
      size_t POS_RADIX = 0; \
      for (size_t J_RADIX = MLOWER_RADIX; J_RADIX <= MUPPER_RADIX; J_RADIX++) \
	{ \
	  const size_t DELTA_RADIX = COUNT_RADIX[J_RADIX]; \
	  COUNT_RADIX[J_RADIX] = POS_RADIX; \
	  POS_RADIX += DELTA_RADIX; \
	} \
      \
      if (!TMP_RADIX) \
	{ \
	  assert (C_RADIX == A_RADIX); \
	  TMP_RADIX = kissat_malloc (solver, BYTES_TMP_RADIX); \
	  B_RADIX = TMP_RADIX; \
	} \
      \
      assert (B_RADIX == TMP_RADIX); \
      \
      VTYPE * D_RADIX = (C_RADIX == A_RADIX) ? B_RADIX : A_RADIX; \
      \
      for (VTYPE * P_RADIX = C_RADIX; P_RADIX != END_RADIX; P_RADIX++) \
	{ \
	  RTYPE R_RADIX = RANK (*P_RADIX); \
	  RTYPE S_RADIX = R_RADIX >> I_RADIX; \
	  RTYPE M_RADIX = S_RADIX & MASK_RADIX; \
	  const size_t POS_RADIX = COUNT_RADIX[M_RADIX]++; \
	  D_RADIX[POS_RADIX] = *P_RADIX; \
	} \
      \
      C_RADIX = D_RADIX; \
    } \
  \
  if (C_RADIX == B_RADIX) \
    memcpy (A_RADIX, B_RADIX, N_RADIX * sizeof *A_RADIX); \
  \
  if (TMP_RADIX) \
    kissat_free (solver, TMP_RADIX, BYTES_TMP_RADIX); \
  \
  CHECK_RANKED (N_RADIX, V_RADIX, RANK); \
  STOP (radix); \
} while (0)

#define RADIX_STACK(LENGTH,VTYPE,RTYPE,S,RANK) \
do { \
  const size_t N_RADIX_STACK = SIZE_STACK (S); \
  VTYPE * A_RADIX_STACK = BEGIN_STACK (S); \
  RADIX(LENGTH,VTYPE,RTYPE,N_RADIX_STACK,A_RADIX_STACK,RANK); \
} while (0)

#endif
