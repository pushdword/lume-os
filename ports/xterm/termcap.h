/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_TERMCAP_H
#define LUME_TERMCAP_H
int tgetent(char *, char *);
int tgetnum(char *);
int tgetflag(char *);
char *tgetstr(char *, char **);
char *tgoto(char *, int, int);
int tputs(char *, int, void (*)(unsigned));
#endif
