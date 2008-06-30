#ifndef INCLUDED_WTERMINAL
#define INCLUDED_WTERMINAL

//
// serial port IOCTL
//

// use with TIOCMBIS
#define TIOCM_RTS 1

// use with TIOCMGET or TIOCMIWAIT
#define TIOCM_CD  0x80	// MS_RLSD_ON
#define TIOCM_CTS 0x10	// MS_CTS_ON

enum
{
	TIOCMBIS,		// set control line
	TIOCMGET,		// get line state
	TIOCMIWAIT		// wait for status change
};

extern int ioctl(int fd, int op, int* data);

#ifndef _WINSOCKAPI_
#define FIONREAD 0
#endif


extern void _get_console(void);
extern void _hide_console(void);


//
// <poll.h>
//

struct pollfd
{
	int fd;
	short int events, revents;
};

#define POLLIN 1

extern int poll(struct pollfd[], int, int);



//
// <termios.h>
//

#define TCSANOW 0

struct termios
{
	long c_lflag;
};

#define ICANON 2	// do not change - correspond to ENABLE_LINE_INPUT / ENABLE_ECHO_INPUT
#define ECHO   4

extern int tcgetattr(int fd, struct termios* termios_p);
extern int tcsetattr(int fd, int optional_actions, const struct termios* termios_p);

#endif	// #ifndef INCLUDED_WTERMINAL
