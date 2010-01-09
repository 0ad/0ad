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

#include "precompiled.h"

#if OS_WIN
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wposix/wsock.h"
#include "lib/sysdep/os/win/wposix/wsock_internal.h"
#endif

#include "lib/sysdep/cpu.h"

#include "Network.h"
#include "NetworkInternal.h"

#include "ps/CStr.h"
// ERROR is defined by some windows header. Undef it
#undef ERROR
#include "ps/CLogger.h"
#include "NetLog.h"

#if !OS_WIN
# include <fcntl.h>
# include <signal.h>
# include <sys/ioctl.h>
#endif

#define LOG_CATEGORY L"net"

// Record global transfer statistics (sent/recvd bytes). This will put a lock
// /unlock pair in all read and write operations.
#define RECORD_GLOBAL_STATS 1

#define GLOBAL_LOCK() pthread_mutex_lock(&g_SocketSetInternal.m_Mutex)
#define GLOBAL_UNLOCK() pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex)

CSocketSetInternal g_SocketSetInternal;

DEFINE_ERROR(NO_SUCH_HOST, "Host not found");
DEFINE_ERROR(CONNECT_TIMEOUT, "The connection attempt timed out");
DEFINE_ERROR(CONNECT_REFUSED, "The connection attempt was refused");
DEFINE_ERROR(NO_ROUTE_TO_HOST, "No route to host");
DEFINE_ERROR(CONNECTION_BROKEN, "The connection has been closed");
DEFINE_ERROR(CONNECT_IN_PROGRESS, "The connect attempt has started, but is not yet complete");
DEFINE_ERROR(PORT_IN_USE, "The port is already in use by another process");
DEFINE_ERROR(INVALID_PORT, "The port specified is either invalid, or forbidden by system or firewall policy");
DEFINE_ERROR(INVALID_PROTOCOL, "The socket type or protocol is not supported by the operating system. Make sure that the TCP/IP protocol is installed and activated");

// Map an OS error number to a PS_RESULT
PS_RESULT GetPS_RESULT(int error)
{
	switch (error)
	{
	case EWOULDBLOCK:
	case EINPROGRESS:
		return PS_OK;
	case ENETUNREACH:
	case ENETDOWN:
	case EADDRNOTAVAIL:
		return NO_ROUTE_TO_HOST;
	case ETIMEDOUT:
		return CONNECT_TIMEOUT;
	case ECONNREFUSED:
		return CONNECT_REFUSED;
	default:
		char buf[256];
		Network_GetErrorString(error, buf, sizeof(buf));
		LOG(CLogger::Error, LOG_CATEGORY, L"SocketBase.cpp::GetPS_RESULT(): Unrecognized error %hs[%d]", buf, error);
		return PS_FAIL;
	}
}

CSocketAddress::CSocketAddress(int port, ESocketProtocol proto)
{
	memset(&m_Union, 0, sizeof(m_Union));
	switch (proto)
	{
	case IPv4:
		m_Union.m_IPv4.sin_family=PF_INET;
		m_Union.m_IPv4.sin_addr.s_addr=htonl(INADDR_ANY);
		m_Union.m_IPv4.sin_port=htons(port);
		break;
	case IPv6:
		m_Union.m_IPv6.sin6_family=PF_INET6;
		cpu_memcpy(&m_Union.m_IPv6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
		m_Union.m_IPv6.sin6_port=htons(port);
		break;
	default:
		debug_warn(L"CSocketAddress::CSocketAddress: Bad proto");
	}
}

CSocketAddress CSocketAddress::Loopback(int port, ESocketProtocol proto)
{
	CSocketAddress ret;
	switch (proto)
	{
	case IPv4:
		ret.m_Union.m_IPv4.sin_family=PF_INET;
		ret.m_Union.m_IPv4.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
		ret.m_Union.m_IPv4.sin_port=htons(port);
		break;
	case IPv6:
		ret.m_Union.m_IPv6.sin6_family=PF_INET6;
		cpu_memcpy(&ret.m_Union.m_IPv6.sin6_addr, &in6addr_loopback, sizeof(in6addr_loopback));
		ret.m_Union.m_IPv6.sin6_port=htons(port);
		break;
	default:
		debug_warn(L"CSocketAddress::CSocketAddress: Bad proto");
	}
	return ret;
}

PS_RESULT CSocketAddress::Resolve(const char *name, int port, CSocketAddress &addr)
{
	// Use IPV4 by default, ignoring address type.
	memset(&addr.m_Union, 0, sizeof(addr.m_Union));
	hostent *he;
	addr.m_Union.m_IPv4.sin_family=AF_INET;
	addr.m_Union.m_IPv4.sin_port=htons(port);
	// Try to parse dot-notation IP
	addr.m_Union.m_IPv4.sin_addr.s_addr=inet_addr(name);
	if (addr.m_Union.m_IPv4.sin_addr.s_addr==INADDR_NONE) // Not a dotted IP, try name resolution
	{
		he=gethostbyname(name);
		if (!he)
			return NO_SUCH_HOST;
		addr.m_Union.m_IPv4.sin_addr=*(struct in_addr *)(he->h_addr_list[0]);
	}
	return PS_OK;
}

CStr CSocketAddress::GetString() const
{
	char convBuf[NI_MAXHOST];
	int res=getnameinfo((struct sockaddr *)&m_Union, sizeof(struct sockaddr_in),
			convBuf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
	if (res == 0)
		return CStr(convBuf);
	// getnameinfo won't return a string for the IPv6 unspecified address
	else if (m_Union.m_Family == IPv6 && res==EAI_NONAME)
		return "::";
	// supported, but failed
	else if (errno != ENOSYS)
		return "";
	// else: IPv6 not supported, fall back to IPv4

	if (m_Union.m_Family == IPv4)
	{
		sprintf_s(convBuf, ARRAY_SIZE(convBuf), "%d.%d.%d.%d",
			m_Union.m_IPv4.sin_addr.s_addr&0xff,
			(m_Union.m_IPv4.sin_addr.s_addr>>8)&0xff,
			(m_Union.m_IPv4.sin_addr.s_addr>>16)&0xff,
			(m_Union.m_IPv4.sin_addr.s_addr>>24)&0xff);
		return CStr(convBuf);
	}
	else
		return CStr();
}

int CSocketAddress::GetPort() const
{
	switch (m_Union.m_Family)
	{
	case IPv4:
	case IPv6:
		return ntohs(m_Union.m_IPv4.sin_port);
	}
	return -1;
}

CSocketBase::CSocketBase()
{
	m_pInternal=new CSocketInternal;
	m_Proto=UNSPEC;
	m_NonBlocking=true;
	m_State=SS_UNCONNECTED;
	m_Error=PS_OK;
}

CSocketBase::CSocketBase(CSocketInternal *pInt)
{
	m_pInternal=pInt;
	m_Proto=pInt->m_RemoteAddr.GetProtocol();
	m_State=SS_CONNECTED;
	m_Error=PS_OK;
	SetNonBlocking(true);
	
	NET_LOG2( "CSocketBase::CSocketBase(): Created socket from fd %d", pInt->m_fd );
}

CSocketBase::~CSocketBase()
{
	NET_LOG4( "CSocketBase::~CSocketBase(): fd is %d. "
					"Received: %lu bytes. Sent: %lu bytes.",
					m_pInternal->m_fd,
					(unsigned long)m_pInternal->m_RecvBytes,
					(unsigned long)m_pInternal->m_SentBytes );

	Destroy();
	delete m_pInternal;
}

void CSocketBase::Shutdown()
{
	GLOBAL_LOCK();
	
	if (g_SocketSetInternal.m_NumSockets)
	{
		NET_LOG2( "CSocketBase::Shutdown(): %lu sockets still open! (forcing network shutdown)", (unsigned long)g_SocketSetInternal.m_NumSockets );
	}

#if RECORD_GLOBAL_STATS
	NET_LOG3( "GLOBAL SOCKET STATISTICS: "
					"Received: %lu bytes. Sent: %lu bytes.",
					(unsigned long)g_SocketSetInternal.m_GlobalRecvBytes,
					(unsigned long)g_SocketSetInternal.m_GlobalSentBytes);
#endif

	GLOBAL_UNLOCK();
	if (g_SocketSetInternal.m_Thread)
	{
		AbortWaitLoop();
		pthread_join(g_SocketSetInternal.m_Thread, NULL);
	}
}

void *WaitLoopThreadMain(void *)
{
	debug_SetThreadName("net_wait");

	GLOBAL_LOCK();
	CSocketBase::RunWaitLoop();
	
	g_SocketSetInternal.m_Thread=0;
	
	GLOBAL_UNLOCK();
	return NULL;
}

PS_RESULT CSocketBase::Initialize(ESocketProtocol proto)
{
	// Use IPV4 by default, ignoring address type.
	int res=socket(AF_INET, SOCK_STREAM, 0);

	NET_LOG2("CSocketBase::Initialize(): socket() res: %d", res);

	if (res == -1)
	{
		return INVALID_PROTOCOL;
	}
	
	m_pInternal->m_fd=res;
	m_Proto=proto;

	GLOBAL_LOCK();
	if (g_SocketSetInternal.m_NumSockets == 0)
		pthread_create(&g_SocketSetInternal.m_Thread, NULL, WaitLoopThreadMain, NULL);
	g_SocketSetInternal.m_NumSockets++;
	GLOBAL_UNLOCK();

	SetNonBlocking(m_NonBlocking);

	return PS_OK;
}

void CSocketBase::Close()
{
	shutdown(m_pInternal->m_fd, SHUT_WR);
	m_State=SS_CLOSED_LOCALLY;
}

void CSocketBase::Destroy()
{
	if (m_pInternal->m_fd == -1)
	{
		m_State=SS_UNCONNECTED;
		return;
	}

	// Remove any data associated with the file descriptor
	GLOBAL_LOCK();
	debug_assert(g_SocketSetInternal.m_NumSockets > 0);

	g_SocketSetInternal.m_NumSockets--;
	g_SocketSetInternal.m_HandleMap.erase(m_pInternal->m_fd);

	if (!g_SocketSetInternal.m_NumSockets)
		AbortWaitLoop();
	GLOBAL_UNLOCK();

	// Disconnect the socket, if it is still connected
	if (m_State == SS_CONNECTED || m_State == SS_CLOSED_LOCALLY)
	{
		// This makes the other end receive a RST, but since
		// we've had no chance to close cleanly and the socket must
		// be destroyed immediately, we've got no choice
		shutdown(m_pInternal->m_fd, SHUT_RDWR);
		m_State=SS_UNCONNECTED;
	}
	// Destroy the socket
	closesocket(m_pInternal->m_fd);
	m_pInternal->m_fd=-1;
}

void CSocketBase::SetNonBlocking(bool nonblocking)
{
	m_NonBlocking=nonblocking;
#if OS_WIN
	unsigned long nb=nonblocking;
	if(!nonblocking)
		SendWaitLoopUpdate();	// Need to call WSAAsyncSelect with event=0 before ioctlsocket
	int res=ioctlsocket(m_pInternal->m_fd, FIONBIO, &nb);
	if (res != 0)
		NET_LOG2("SetNonBlocking: res %d", res);
#else
	int oldflags=fcntl(m_pInternal->m_fd, F_GETFL, 0);
	if (oldflags != -1)
	{
		if (nonblocking)
			oldflags |= O_NONBLOCK;
		else
			oldflags &= ~O_NONBLOCK;
		fcntl(m_pInternal->m_fd, F_SETFL, oldflags);
	}
#endif
}

void CSocketBase::SetTcpNoDelay(bool tcpNoDelay)
{
	// Disable Nagle's Algorithm
	int data=tcpNoDelay;
	setsockopt(m_pInternal->m_fd, SOL_SOCKET, TCP_NODELAY, (const char *)&data, sizeof(data));
}

PS_RESULT CSocketBase::Read(void *buf, size_t len, size_t *bytesRead)
{
	int res;
	char errbuf[256];
	
	res=recv(m_pInternal->m_fd, (char *)buf, len, 0);
	if (res < 0)
	{
		*bytesRead=0;
		int error=Network_LastError;
		switch (error)
		{
		case EWOULDBLOCK:
			return PS_OK;
		/*case ENETDOWN:
		case ENETRESET:
		case ENOTCONN:
		case ESHUTDOWN:
		case ECONNABORTED:
		case ECONNRESET:
		case ETIMEDOUT:*/
		default:
			Network_GetErrorString(error, errbuf, sizeof(errbuf));
			NET_LOG3("Read error %s [%d]", errbuf, error);
			m_State=SS_UNCONNECTED;
			m_Error=GetPS_RESULT(error);
			return m_Error;
		}
	}

	if (res == 0 && len > 0) // EOF - Cleanly closed socket
	{
		*bytesRead=0;
		m_State=SS_UNCONNECTED;
		m_Error=PS_OK;
		return CONNECTION_BROKEN;
	}
	
	*bytesRead=res;
	
	m_pInternal->m_RecvBytes += res;
#if RECORD_GLOBAL_STATS
	GLOBAL_LOCK();
	g_SocketSetInternal.m_GlobalRecvBytes += res;
	GLOBAL_UNLOCK();
#endif
	
	return PS_OK;
}

PS_RESULT CSocketBase::Write(void *buf, size_t len, size_t *bytesWritten)
{
	int res;
	char errbuf[256];

	res=send(m_pInternal->m_fd, (char *)buf, len, 0);
	if (res < 0)
	{
		*bytesWritten=0;
		int err=Network_LastError;
		switch (err)
		{
		case EWOULDBLOCK:
			return PS_OK;
		/*case ENETDOWN:
		case ENETRESET:
		case ENOTCONN:
		case ESHUTDOWN:
		case ECONNABORTED:
		case ECONNRESET:
		case ETIMEDOUT:
		case EHOSTUNREACH:*/
		default:
			Network_GetErrorString(err, errbuf, sizeof(errbuf));
			NET_LOG3("Write error %s [%d]", errbuf, err);
			m_State=SS_UNCONNECTED;
			return CONNECTION_BROKEN;
		}
	}

	*bytesWritten=res;

	m_pInternal->m_SentBytes += res;
#if RECORD_GLOBAL_STATS
	GLOBAL_LOCK();
	g_SocketSetInternal.m_GlobalSentBytes += res;
	GLOBAL_UNLOCK();
#endif
	
	return PS_OK;
}

PS_RESULT CSocketBase::Connect(const CSocketAddress &addr)
{
	int res = connect(m_pInternal->m_fd, (struct sockaddr *)(&addr.m_Union), sizeof(struct sockaddr_in));
	NET_LOG3("connect returned %d [%d]", res, m_NonBlocking);

	if (res != 0)
	{
		int error=Network_LastError;
		NET_LOG2("last error was %d", error);
		if (m_NonBlocking && error == EWOULDBLOCK)
		{
			m_State=SS_CONNECT_STARTED;
		}
		else
		{
			m_State=SS_UNCONNECTED;
			m_Error=GetPS_RESULT(error);
		}
	}
	else
	{
		m_State=SS_CONNECTED;
		m_Error=PS_OK;
	}

	return m_Error;
}

PS_RESULT CSocketBase::Bind(const CSocketAddress &address)
{
	char errBuf[256];
	int res;

	Initialize(address.GetProtocol());

	SetOpMask(READ);

	res=bind(m_pInternal->m_fd, (const struct sockaddr*)&address, (socklen_t)sizeof(struct sockaddr_in));
	if (res == -1)
	{
		PS_RESULT ret=PS_FAIL;
		int err=Network_LastError;
		switch (err)
		{
			case EADDRINUSE:
				ret=PORT_IN_USE;
				break;
			case EACCES:
			case EADDRNOTAVAIL:
				ret=INVALID_PORT;
				break;
			default:
				Network_GetErrorString(err, errBuf, sizeof(errBuf));
				LOG(CLogger::Error, LOG_CATEGORY, L"CServerSocket::Bind(): bind: %hs [%d] => PS_FAIL", errBuf, err);
		}
		m_State=SS_UNCONNECTED;
		m_Error=ret;
		return ret;
	}

	res=listen(m_pInternal->m_fd, 5);
	if (res == -1)
	{
		int err=Network_LastError;
		Network_GetErrorString(err, errBuf, sizeof(errBuf));
		LOG(CLogger::Error, LOG_CATEGORY, L"CServerSocket::Bind(): listen: %hs [%d] => PS_FAIL", errBuf, err);
		m_State=SS_UNCONNECTED;
		return PS_FAIL;
	}

	m_State=SS_CONNECTED;
	m_Error=PS_OK;
	return PS_OK;
}

PS_RESULT CSocketBase::PreAccept(CSocketAddress &addr)
{
	socklen_t addrLen=sizeof(struct sockaddr_in);
	int fd=accept(m_pInternal->m_fd, (struct sockaddr *)&addr.m_Union, &addrLen);
	m_pInternal->m_AcceptFd=fd;
	m_pInternal->m_AcceptAddr=addr;
	if (fd != -1)
		return PS_OK;
	else
	{
		PS_RESULT res=GetPS_RESULT(Network_LastError);
		// GetPS_RESULT considers some errors non-failures
		if (res == PS_OK)
			return PS_FAIL;
		else
			return res;
	}
}

CSocketInternal *CSocketBase::Accept()
{
	if (m_pInternal->m_AcceptFd != -1)
	{
		CSocketInternal *pInt=new CSocketInternal();
		pInt->m_fd=m_pInternal->m_AcceptFd;
		pInt->m_RemoteAddr=m_pInternal->m_AcceptAddr;
		
		GLOBAL_LOCK();
		g_SocketSetInternal.m_NumSockets++;		
		GLOBAL_UNLOCK();

		m_pInternal->m_AcceptFd=-1;
		return pInt;
	}
	else
		return NULL;
}

void CSocketBase::Reject()
{
	shutdown(m_pInternal->m_AcceptFd, SHUT_RDWR);
	closesocket(m_pInternal->m_AcceptFd);
}

// UNIX select loop
#if !OS_WIN
// ConnectError is called on a socket the first time it selects as ready
// after the BeginConnect, to check errors on the socket and update the
// connection status information
//
// Returns: true if error callback should be called, false if it should not
bool CSocketBase::ConnectError(CSocketBase *pSocket)
{
	CSocketInternal *pInt=pSocket->m_pInternal;
	size_t buf;
	int res;
	
	if (pSocket->m_State==SS_CONNECT_STARTED)
	{
		res=read(pInt->m_fd, &buf, 0);
		// read of zero bytes should be a successful no-op, unless
		// there was an error
		if (res == -1)
		{
			pSocket->m_State=SS_UNCONNECTED;
			PS_RESULT connErr=GetPS_RESULT(errno);
			NET_LOG4("Connect error: %s [%d:%s]", connErr, errno, strerror(errno));
			pSocket->m_Error=connErr;
			return true;
		}
		else
		{
			pSocket->m_State=SS_CONNECTED;
			pSocket->m_Error=PS_OK;
		}
	}
	
	return false;
}

// SocketWritable is called whenever a socket selects as writable in the unix
// select loop. This will call the callback after checking for a connect error
// if there's a connect in progress, as well as update the socket's state.
//
// Locking: The global mutex must be held when entering this function, and it
// will be held upon return.
void CSocketBase::SocketWritable(CSocketBase *pSock)
{
	//CSocketInternal *pInt=pSock->m_pInternal;
	bool isConnectError=false;

	if (pSock->m_State != SS_CONNECTED)
		isConnectError=ConnectError(pSock);

	GLOBAL_UNLOCK();

	if (isConnectError)
		pSock->OnClose(pSock->m_Error);
	else
		pSock->OnWrite();

	GLOBAL_LOCK();
}

// SocketReadable is called whenever a socket selects as writable in the unix
// select loop. This will call the callback after checking for a connect error
// if there's a connect in progress, as well as update the socket's state.
//
// Locking: The global mutex must be held when entering this function, and it
// will be held upon return.
void CSocketBase::SocketReadable(CSocketBase *pSock)
{
	bool isError=false;

	if (pSock->m_State == SS_CONNECT_STARTED)
		isError=ConnectError(pSock);
	else if (pSock->m_State == SS_UNCONNECTED)
	{
		// UNCONNECTED sockets don't get callbacks
		// Note that server sockets that are bound have state==SS_CONNECTED
		return;
	}
	else if (pSock->m_State != SS_UNCONNECTED)
	{
		size_t nRead;
		errno=0;
		int res=ioctl(pSock->m_pInternal->m_fd, FIONREAD, &nRead);
		// failure, errno=EINVAL means server socket
		// success, nRead != 0 means alive stream socket
		if (res == -1 && errno != EINVAL)
		{
			NET_LOG3("RunWaitLoop:ioctl: Connection broken [%d:%s]", errno, strerror(errno));
			// Don't use API function - we both hold a lock and
			// it is unnecessary to SendWaitLoopUpdate at this
			// stage
			pSock->m_pInternal->m_Ops=0;
			pSock->m_State=SS_UNCONNECTED;
			if (errno)
				pSock->m_Error=GetPS_RESULT(errno);
			else
				pSock->m_Error=PS_OK;
			isError=true;
		}
	}

	GLOBAL_UNLOCK();

	if (isError)
		pSock->OnClose(pSock->m_Error);
	else
		pSock->OnRead();

	GLOBAL_LOCK();
}

void CSocketBase::RunWaitLoop()
{
	int res;

	signal(SIGPIPE, SIG_IGN);

	// Create Control Pipe
	res=pipe(g_SocketSetInternal.m_Pipe);
	if (res != 0)
		return;
	
	// The lock is held upon entry and exit of this loop. There are a few places
	// where the lock is released and then re-acquired: when calling callbacks
	// and when calling select().
	while (true)
	{
		std::map<int, CSocketBase *>::iterator it;
		fd_set rfds;
		fd_set wfds;
		int fd_max=g_SocketSetInternal.m_Pipe[0];

		// Prepare fd_set: Read
		FD_ZERO(&rfds);
		FD_SET(g_SocketSetInternal.m_Pipe[0], &rfds);

		// Prepare fd_set: Write
		FD_ZERO(&wfds);

		it=g_SocketSetInternal.m_HandleMap.begin();
		while (it != g_SocketSetInternal.m_HandleMap.end())
		{
			size_t ops=it->second->m_pInternal->m_Ops;

			if (ops && it->first > fd_max)
				fd_max=it->first;
			if (ops & READ)
				FD_SET(it->first, &rfds);
			if (ops & WRITE)
				FD_SET(it->first, &wfds);

			++it;
		}
		GLOBAL_UNLOCK();

		// select, timeout infinite
		res=select(fd_max+1, &rfds, &wfds, NULL, NULL);

		GLOBAL_LOCK();
		// Check select error
		if (res == -1)
		{
			// It is possible for a socket to be deleted between preparing the
			// fd_sets and actually performing the select - in which case it
			// will fire a Bad file descriptor error. Simply retry.
			if (Network_LastError == EBADF)
				continue;
			perror("CSocketSet::RunWaitLoop(), select");
			continue;
		}

		// Check Control Pipe
		if (FD_ISSET(g_SocketSetInternal.m_Pipe[0], &rfds))
		{
			char bt;
			if (read(g_SocketSetInternal.m_Pipe[0], &bt, 1) == 1)
			{
				if (bt=='q')
					break;
				else if (bt=='r')
				{
					// Reload sockets - just skip to the beginning of the loop
					continue;
				}
			}
			
			FD_CLR(g_SocketSetInternal.m_Pipe[0], &rfds);
		}

		// Go through sockets
		int i=-1;
		while (++i <= fd_max)
		{
			if (!FD_ISSET(i, &rfds) && !FD_ISSET(i, &wfds))
				continue;
			
			it=g_SocketSetInternal.m_HandleMap.find(i);
			if (it == g_SocketSetInternal.m_HandleMap.end())
				continue;
			
			CSocketBase *pSock=it->second;
			
			if (FD_ISSET(i, &wfds))
			{
				SocketWritable(pSock);
			}

			// After the callback is called, we must check if the socket
			// still exists (sockets may delete themselves in the callback)
			it=g_SocketSetInternal.m_HandleMap.find(i);
			if (it == g_SocketSetInternal.m_HandleMap.end())
				continue;

			if (FD_ISSET(i, &rfds))
			{
				SocketReadable(pSock);
			}
		}
	}
	
	// Close control pipe
	close(g_SocketSetInternal.m_Pipe[0]);
	close(g_SocketSetInternal.m_Pipe[1]);

	return;
}

void CSocketBase::SendWaitLoopAbort()
{
	char msg='q';
	write(g_SocketSetInternal.m_Pipe[1], &msg, 1);
}

void CSocketBase::SendWaitLoopUpdate()
{
//	NET_LOG("SendWaitLoopUpdate: fd %d, ops %u\n", m_pInternal->m_fd, m_pInternal->m_Ops);
	char msg='r';
	write(g_SocketSetInternal.m_Pipe[1], &msg, 1);
}

// Windows WindowProc for async event notification
#else	// i.e. #if OS_WIN

void WaitLoop_SocketUpdateProc(int fd, int error, int event)
{
	GLOBAL_LOCK();
	CSocketBase *pSock=g_SocketSetInternal.m_HandleMap[fd];
	GLOBAL_UNLOCK();
	
	// FIXME What if the fd isn't in the handle map?

	if (error)
	{
		PS_RESULT res=GetPS_RESULT(error);
		pSock->m_Error=res;
		pSock->m_State=SS_UNCONNECTED;
		if (res == PS_FAIL)
			pSock->OnClose(CONNECTION_BROKEN);
		return;
	}

	if (pSock->m_State==SS_CONNECT_STARTED)
	{
		pSock->m_Error=PS_OK;
		pSock->m_State=SS_CONNECTED;
	}

	switch (event)
	{
	case FD_ACCEPT:
	case FD_READ:
		pSock->OnRead();
		break;
	case FD_CONNECT:
	case FD_WRITE:
		pSock->OnWrite();
		break;
	case FD_CLOSE:
		// If FD_CLOSE and error, OnClose has already been called above
		// with the appropriate PS_RESULT
		pSock->m_State=SS_UNCONNECTED;
		pSock->OnClose(PS_OK);
		break;
	}
}

LRESULT WINAPI WaitLoop_WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//printf("WaitLoop_WindowProc(): Windows message: %d:%d:%d\n", msg, wParam, lParam);
	switch (msg)
	{
		case MSG_SOCKET_READY:
		{
			int event=LOWORD(lParam);
			int error=HIWORD(lParam);
			
			WaitLoop_SocketUpdateProc((int)wParam, error?error-WSABASEERR:0, event);
			return FALSE;
		}
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

// Locked on entry, must be locked on exit
void CSocketBase::RunWaitLoop()
{
	int ret;
	char errBuf[256] = {0};
	MSG msg;

	WNDCLASS wc;
	ATOM atom;

	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpszClassName="Network Event WindowClass";
	wc.lpfnWndProc=WaitLoop_WindowProc;

	atom=RegisterClass(&wc);
	if (!atom)
	{
		ret=GetLastError();
		Network_GetErrorString(ret, (LPSTR)&errBuf, 256);
		NET_LOG3("RegisterClass: %s [%d]", errBuf, ret);
		return;
	}

	// Create message window
	g_SocketSetInternal.m_hWnd=CreateWindow((LPCTSTR)atom, "Network Event Window", WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);

	if (!g_SocketSetInternal.m_hWnd)
	{
		ret=GetLastError();
		Network_GetErrorString(ret, errBuf, sizeof(errBuf));
		NET_LOG3("CreateWindowEx: %s [%d]", errBuf, ret);
		return;
	}
	
	// If OpMasks where set in another thread before we got this far,
	// WSAAsyncSelect will need to be called again
	std::map<int, CSocketBase *>::iterator it;
	it=g_SocketSetInternal.m_HandleMap.begin();
	while (it != g_SocketSetInternal.m_HandleMap.end())
	{
		it->second->SetOpMask(it->second->GetOpMask());
		++it;
	}

	NET_LOG2("Commencing message loop. hWnd %p", g_SocketSetInternal.m_hWnd);
	GLOBAL_UNLOCK();
	
	while ((ret=GetMessage(&msg, g_SocketSetInternal.m_hWnd, 0, 0))!=0)
	{
		//printf("RunWaitLoop(): Windows message: %d:%d:%d\n", msg.message, msg.wParam, msg.lParam);
		if (ret == -1)
		{
			ret=GetLastError();
			Network_GetErrorString(ret, errBuf, sizeof(errBuf));
			NET_LOG3("GetMessage: %s [%d]", errBuf, ret);
		}
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	GLOBAL_LOCK();
	g_SocketSetInternal.m_Thread=0;
	// FIXME Leak: Destroy window
	g_SocketSetInternal.m_hWnd=0;

	NET_LOG("RunWaitLoop returning");
	return;
}

void CSocketBase::SendWaitLoopAbort()
{
	if (g_SocketSetInternal.m_hWnd)
	{
		PostMessage(g_SocketSetInternal.m_hWnd, WM_QUIT, 0, 0);
	}
	else
		NET_LOG("SendWaitLoopUpdate: No WaitLoop Running.");
}

void CSocketBase::SendWaitLoopUpdate()
{
	GLOBAL_LOCK();
	if (g_SocketSetInternal.m_hWnd)
	{
		if(m_NonBlocking == false)
		{
			GLOBAL_UNLOCK();
			WSAAsyncSelect(m_pInternal->m_fd, g_SocketSetInternal.m_hWnd, MSG_SOCKET_READY, 0);
			return;
		}

		long wsaOps=FD_CLOSE;
		if (m_pInternal->m_Ops & READ)
			wsaOps |= FD_READ|FD_ACCEPT;
		if (m_pInternal->m_Ops & WRITE)
			wsaOps |= FD_WRITE|FD_CONNECT;
		GLOBAL_UNLOCK();
		//printf("SendWaitLoopUpdate: %d: %u %x -> %p\n", m_pInternal->m_fd, m_pInternal->m_Ops, wsaOps, g_SocketSetInternal.m_hWnd);
		WSAAsyncSelect(m_pInternal->m_fd, g_SocketSetInternal.m_hWnd, MSG_SOCKET_READY, wsaOps);
	}
	else
	{
		//printf("SendWaitLoopUpdate: No WaitLoop Running.\n");
		GLOBAL_UNLOCK();
	}
}
#endif	// #if OS_WIN

void CSocketBase::AbortWaitLoop()
{
	SendWaitLoopAbort();
//	pthread_join(g_SocketSetInternal.m_Thread);
}

int CSocketBase::GetOpMask()
{
	return m_pInternal->m_Ops;
}

void CSocketBase::SetOpMask(int ops)
{
	GLOBAL_LOCK();
	g_SocketSetInternal.m_HandleMap[m_pInternal->m_fd]=this;
	m_pInternal->m_Ops=ops;
	
	/*printf("SetOpMask(fd %d, ops %u) %u\n",
		m_pInternal->m_fd,
		ops,
		g_SocketSetInternal.m_HandleMap[m_pInternal->m_fd]->m_pInternal->m_Ops);*/

	GLOBAL_UNLOCK();
	
	SendWaitLoopUpdate();
}
