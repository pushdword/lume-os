#ifndef LUME_HTOP_TERM_H
#define LUME_HTOP_TERM_H
extern char *enter_ca_mode, *exit_ca_mode, *clear_screen;
int tputs(const char *, int, int (*)(int));
#endif
