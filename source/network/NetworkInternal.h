/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_NETWORK_NETWORKINTERNAL
#define INCLUDED_NETWORK_NETWORKINTERNAL

#include <map>

#if !OS_WIN

#define Network_GetErrorString(_error, _buf, _buflen) strerror_r(_error, _buf, _buflen)

#define Network_LastError errno

#define closesocket(_fd) close(_fd)

#else	// i.e. #if OS_WIN

#define Network_GetErrorString(_error, _buf, _buflen) \
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, _error+WSABASEERR, 0, _buf, _buflen, NULL)
#define Network_LastError (WSAGetLastError() - WSABASEERR)
// These are defined so that WSAGLE - WSABASEERR = E*
// i.e. the same error name can be used in winsock and posix
#define WSABASEERR			10000

#define MSG_SOCKET_READY WM_USER

#endif	// #if OS_WIN

typedef int socket_t;

class CSocketInternal
{
public:
	socket_t m_fd;
	CSocketAddress m_RemoteAddr;

	socket_t m_AcceptFd;
	CSocketAddress m_AcceptAddr;
	
	// Bitwise OR of all operations to listen for.
	// See READ and WRITE
	int m_Ops;

	char *m_pConnectHost;
	int m_ConnectPort;
	
	u64 m_SentBytes;
	u64 m_RecvBytes;

	inline CSocketInternal():
		m_fd(-1),
		m_AcceptFd(-1),
		m_Ops(0),
		m_pConnectHost(NULL),
		m_ConnectPort(-1),
		m_SentBytes(0),
		m_RecvBytes(0)
	{
	}
};

struct CSocketSetInternal
{
	// Any access to the global variables should be protected using m_Mutex
	pthread_mutex_t m_Mutex;
	pthread_t m_Thread;
	
	size_t m_NumSockets;

	std::map <socket_t, CSocketBase * > m_HandleMap;
#if OS_WIN
	HWND m_hWnd;
#else
	// [0] is for use by RunWaitLoop, [1] for SendWaitLoopAbort and SendWaitLoopUpdate
	int m_Pipe[2];
#endif

	u64 m_GlobalSentBytes;
	u64 m_GlobalRecvBytes;

public:
	inline CSocketSetInternal()
	{
#if OS_WIN
		m_hWnd=NULL;
#else
		m_Pipe[0]=-1;
		m_Pipe[1]=-1;
#endif
		pthread_mutex_init(&m_Mutex, NULL);
		m_Thread=0;
		m_NumSockets=0;
		m_GlobalSentBytes=0;
		m_GlobalRecvBytes=0;
	}
};

#endif
