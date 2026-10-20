#ifndef LUME_SSH_IOCTL_H
#define LUME_SSH_IOCTL_H

struct winsize
{
    unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel;
};

#define TIOCSWINSZ 0x5414
#define TIOCGWINSZ 0x5413
#define TIOCSCTTY 0x540e
#define FIONREAD 0x541b
int ioctl(int, unsigned long, ...);
#endif
