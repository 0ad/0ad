#ifndef _NetworkInternal_H
#define _NetworkInternal_H

#include <map>

#ifndef _WIN32

#define Network_GetErrorString(_error, _buf, _buflen) strerror_r(_error, _buf, _buflen)

#define Network_LastError errno

#define closesocket(_fd) close(_fd)
// WSA error codes, with their POSIX counterpart.
#define mkec(_nm) Network_##_nm = _nm

#else

#include "win.h"
IMP(int, WSAAsyncSelect, (int s, HANDLE hWnd, uint wMsg, long lEvent))

#define FD_READ_BIT      0
#define FD_READ          (1 << FD_READ_BIT)

#define FD_WRITE_BIT     1
#define FD_WRITE         (1 << FD_WRITE_BIT)

#define FD_ACCEPT_BIT    3
#define FD_ACCEPT        (1 << FD_ACCEPT_BIT)

#define FD_CONNECT_BIT   4
#define FD_CONNECT       (1 << FD_CONNECT_BIT)

#define FD_CLOSE_BIT     5
#define FD_CLOSE         (1 << FD_CLOSE_BIT)

// Under linux/posix, these have defined values of 0, 1 and 2
// but the WS docs say nothing - so we treat them as unknown
/*enum {
	SHUT_RD=SD_RECEIVE,
	SHUT_WR=SD_SEND,
	SHUT_RDWR=SD_BOTH
};*/
#define Network_GetErrorString(_error, _buf, _buflen) \
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, _error+WSABASEERR, 0, _buf, _buflen, NULL)
#define Network_LastError (WSAGetLastError() - WSABASEERR)
#define mkec(_nm) Network_##_nm = /*WSA##*/_nm
// These are defined so that WSAGLE - WSABASEERR = E*
// i.e. the same error name can be used in winsock and posix
#define WSABASEERR			10000

#define EWOULDBLOCK          (35)
#define ENETDOWN             (50)
#define ENETUNREACH          (51)
#define ENETRESET            (52)
#define ENOTCONN             (57)
#define ESHUTDOWN            (58)
#define ENOTCONN             (57)
#define ECONNABORTED         (53)
#define ECONNRESET           (54)
#define ETIMEDOUT            (60)
#define EADDRINUSE           (48)
#define EADDRNOTAVAIL        (49)
#define ECONNREFUSED         (61)
#define EHOSTUNREACH         (65)

#define MSG_SOCKET_READY WM_USER

#endif

typedef int socket_t;

class CSocketInternal
{
public:
	socket_t m_fd;
	SocketAddress m_RemoteAddr;

	socket_t m_AcceptFd;
	SocketAddress m_AcceptAddr;
	
	// Bitwise OR of all operations to listen for.
	// See READ and WRITE
	uint m_Ops;

	char *m_pConnectHost;
	int m_ConnectPort;

	inline CSocketInternal():
		m_fd(-1),
		m_AcceptFd(-1),
		m_Ops(0),
		m_pConnectHost(NULL),
		m_ConnectPort(-1)
	{
	}
};

struct CSocketSetInternal
{
	// Any access to the global variables should be protected using m_Mutex
	pthread_mutex_t m_Mutex;
	pthread_t m_Thread;

	std::map <socket_t, CSocketBase *> m_HandleMap;
#ifdef _WIN32
	HWND m_hWnd;
#else
	// [0] is for use by RunWaitLoop, [1] for SendWaitLoopAbort and SendWaitLoopUpdate
	int m_Pipe[2];
#endif

public:
	inline CSocketSetInternal()
	{
#ifdef _WIN32
		m_hWnd=NULL;
#else
		m_Pipe[0]=-1;
		m_Pipe[1]=-1;
#endif
		pthread_mutex_init(&m_Mutex, NULL);
		m_Thread=0;
	}
};

#endif
