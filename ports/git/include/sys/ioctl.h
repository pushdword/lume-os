#ifndef LUME_GIT_IOCTL_H
#define LUME_GIT_IOCTL_H
#define TIOCGWINSZ 0x5413
#define FIONBIO 0x5421

struct winsize
{
    unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel;
};

int ioctl(int, unsigned long, ...);
#endif
