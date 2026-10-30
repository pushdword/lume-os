#ifndef LUME_SHELL_JOBS_H
#define LUME_SHELL_JOBS_H
void shell_jobs_initialize(int);
int shell_jobs_begin(const char *, int);
int shell_jobs_active(void);
int shell_jobs_spawn(const char *, unsigned, const char *const *, unsigned, const char *const *);
int shell_jobs_complete(void);
void shell_jobs_poll(void);
void shell_jobs_shutdown(void);
int shell_jobs_builtin(int, char **, unsigned *);
#endif
