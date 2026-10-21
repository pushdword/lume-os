#ifndef LUME_SSH_TERMINAL_H
#define LUME_SSH_TERMINAL_H
void ssh_terminal_register(int, int, int);
void ssh_terminal_forget(int);
unsigned ssh_terminal_pending(int);
int ssh_terminal_read(int, void *, unsigned);
#endif
