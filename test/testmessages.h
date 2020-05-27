#ifndef _testmessages_h_INCLUDED
#define _testmessages_h_INCLUDED

extern int tissat_verbosity;
extern int tissat_warnings;

void tissat_bold_message (const char *, ...);
void tissat_error (const char *, ...);
void tissat_fatal (const char *, ...);
void tissat_line (void);
void tissat_message (const char *, ...);
void tissat_section (const char *, ...);
void tissat_signal (int sig, const char *, ...);
void tissat_verbose (const char *, ...);
void tissat_warning (const char *, ...);

#endif
