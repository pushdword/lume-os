#ifndef LUME_PORT_IOCTL_H
#define LUME_PORT_IOCTL_H

struct winsize
{
    unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel;
};

#define TIOCGWINSZ 0x5413
#ifdef __cplusplus
extern "C"
{
#endif
    int ioctl(int, unsigned long, ...);
#ifdef __cplusplus
}
#endif
#endif
