#ifndef _testdivert_h_INCLUDED
#define _testdivert_h_INCLUDED

void tissat_divert_stdout_and_stderr_to_dev_null (void);
void tissat_restore_stdout_and_stderr (void);

void tissat_restore_stderr (void);
void tissat_redirect_stderr_to_stdout (void);

#endif
