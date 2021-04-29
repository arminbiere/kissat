#ifndef _kitten_h_INCLUDED
#define _kitten_h_INCLUDED

#include <stdint.h>
#include <stdlib.h>

typedef struct kitten kitten;

kitten *kitten_init (void);
void kitten_clear (kitten *);
void kitten_release (kitten *);

void kitten_track_antecedents (kitten *);
void kitten_shuffle (kitten *);

void kitten_clause (kitten *, unsigned id, size_t size, const unsigned *);

int kitten_solve (kitten *);

signed char kitten_value (kitten *, unsigned);

unsigned kitten_compute_clausal_core (kitten *, uint64_t * learned);
void kitten_shrink_to_clausal_core (kitten *);

void kitten_traverse_clausal_core (kitten *, void *state,
				   void (*traverse) (void *state,
						     unsigned id));

void kitten_traverse_core_lemmas (kitten *, void *state,
				  void (*traverse) (void *state,
						    size_t,
						    const unsigned *));
struct kissat;
kitten *kitten_embedded (struct kissat *);

#endif
