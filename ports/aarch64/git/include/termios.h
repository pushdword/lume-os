#ifndef LUME_GIT_TERMIOS_H
#define LUME_GIT_TERMIOS_H
typedef unsigned int tcflag_t;
typedef unsigned char cc_t;
typedef unsigned int speed_t;
#define NCCS 32

struct termios
{
    tcflag_t c_iflag, c_oflag, c_cflag, c_lflag;
    cc_t c_cc[NCCS];
    speed_t c_ispeed, c_ospeed;
};

#define ISIG 1
#define ICANON 2
#define ECHO 8
#define ECHOE 16
#define ECHOK 32
#define ECHONL 64
#define IEXTEN 32768
#define VTIME 5
#define VMIN 6
#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2
int tcgetattr(int, struct termios *);
int tcsetattr(int, int, const struct termios *);
#endif
