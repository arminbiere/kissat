#ifndef _handle_h_INCLUDED
#define _handle_h_INCLUDED

void kissat_init_signal_handler (void (*handler) (int));
void kissat_reset_signal_handler (void);

void kissat_init_alarm (void (*handler) (void));
void kissat_reset_alarm (void);

const char *kissat_signal_name (int sig);

#endif
