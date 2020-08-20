#ifndef _error_h_INCLUDED
#define _error_h_INCLUDED

void kissat_error (const char *fmt, ...);
void kissat_fatal (const char *fmt, ...);

void kissat_fatal_message_start (void);

void kissat_call_function_instead_of_abort (void (*)(void));
void kissat_abort ();

#endif
