#ifndef LUME_SSH_TERMIOS_H
#define LUME_SSH_TERMIOS_H
#include <stdint.h>
typedef uint32_t tcflag_t;
typedef uint32_t speed_t;
typedef unsigned char cc_t;
#define NCCS 32

struct termios
{
    tcflag_t c_iflag, c_oflag, c_cflag, c_lflag;
    cc_t c_cc[NCCS];
    speed_t c_ispeed, c_ospeed;
};

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2
#define TCIFLUSH 0
#define TCIOFLUSH 2
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16
#define ISIG 1
#define ICANON 2
#define ECHO 8
#define ECHOE 16
#define ECHOK 32
#define ECHONL 64
#define NOFLSH 128
#define TOSTOP 256
#define IEXTEN 32768
#define IGNBRK 1
#define BRKINT 2
#define IGNPAR 4
#define PARMRK 8
#define INPCK 16
#define ISTRIP 32
#define INLCR 64
#define IGNCR 128
#define ICRNL 256
#define IXON 1024
#define IXANY 2048
#define IXOFF 4096
#define OPOST 1
#define ONLCR 4
#define OCRNL 8
#define ONOCR 16
#define ONLRET 32
#define CSIZE 48
#define CS7 32
#define CS8 48
#define PARENB 256
#define PARODD 512
#define B0 0
#define B9600 13
#define B19200 14
#define B38400 15
#define B57600 4097
#define B115200 4098
#define B230400 4099
#define B4000000 4111
int tcgetattr(int, struct termios *);
int tcsetattr(int, int, const struct termios *);
speed_t cfgetispeed(const struct termios *);
speed_t cfgetospeed(const struct termios *);
int cfsetispeed(struct termios *, speed_t);
int cfsetospeed(struct termios *, speed_t);
void cfmakeraw(struct termios *);
int tcflush(int, int);
#endif
