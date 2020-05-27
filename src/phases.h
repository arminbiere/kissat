#ifndef _phases_h_INCLUDED
#define _phases_h_INCLUDED

typedef struct phase phase;

struct phase
{
  signed char best:2;
  signed char saved:2;
  signed char target:2;
};

#define PHASE(IDX) \
  (assert ((IDX) < VARS), (solver->phases + (IDX)))

#define BEST(IDX) (PHASE(IDX)->best)
#define SAVED(IDX) (PHASE(IDX)->saved)
#define TARGET(IDX) (PHASE(IDX)->target)

struct kissat;

void kissat_save_best_phases (struct kissat *);
void kissat_save_target_phases (struct kissat *);
void kissat_clear_target_phases (struct kissat *);

#define all_phases(P) \
  phase * P = solver->phases, * END_ ## P = P + VARS; P != END_ ## P; P++

#endif
