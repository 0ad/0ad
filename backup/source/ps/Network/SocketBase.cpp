#include "Network.h"
#include "NetworkInternal.h"

#include "lib.h"
#include "CStr.h"

#include <errno.h>

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
		memcpy(&m_Union.m_IPv6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
		m_Union.m_IPv6.sin6_port=htons(port);
		break;
	}
}

PS_RESULT CSocketAddress::Resolve(const char *name, int port, CSocketAddress &addr)
{
	if ((getaddrinfo) != NULL)
	{
		addrinfo *ai;
		int res=getaddrinfo(name, NULL, NULL, &ai);
		if (res == 0)
		{
			if (ai->ai_addrlen < sizeof(addr.m_Union))
				memcpy(&addr.m_Union, ai->ai_addr, ai->ai_addrlen);
			switch (addr.m_Union.m_Family)
			{
			case IPv4:
				addr.m_Union.m_IPv4.sin_port=htons(port);
				break;
			case IPv6:
				addr.m_Union.m_IPv6.sin6_port=htons(port);
				break;
			}
			freeaddrinfo(ai);
			return PS_OK;
		}
		else
			return NO_SUCH_HOST;
	}
	else
	{
 		hostent *he;
	 
  		addr.m_Union.m_IPv4.sin_family=AF_INET;
 		addr.m_Union.m_IPv4.sin_port=htons(port);
		// Try to parse dot-notation IP
 		addr.m_Union.m_IPv4.sin_addr.s_addr=inet_addr(name);
 		if (addr.m_Union.m_IPv4.sin_addr.s_addr==INADDR_NONE) // Not a dotted IP, try name resolution
 		{
 			he=gethostbyname(name);
 			if (!he)
 			{
 				return NO_SUCH_HOST;
			}
 			addr.m_Union.m_IPv4.sin_addr=*(struct in_addr *)(he->h_addr_list[0]);
		}
		return PS_OK;
	}
}

CStr CSocketAddress::GetString() const
{
	char convBuf[NI_MAXHOST];
	if ((getnameinfo) != NULL)
	{
		int res=getnameinfo((sockaddr *)&m_Union, sizeof(m_Union), convBuf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
		if (res == 0)
		{
			return CStr(convBuf);
		}
		// getnameinfo won't return a string for the IPv6 unspecified address
		else if (m_Union.m_Family == IPv6 && res==EAI_NONAME)
			return "::";
		else
			return "";
	}
	else if (m_Union.m_Family == IPv4)
	{
		sprintf(convBuf, "%d.%d.%d.%d",
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
}

CSocketBase::~CSocketBase()
{
	// Remove any associated data from the CSocketSet
	pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);
	
	g_SocketSetInternal.m_HandleMap.erase(m_pInternal->m_fd);
	
	pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);
	// Disconnect the socket, if it is still connected
	if (m_State == SS_CONNECTED)
	{
		// This makes the other end receive a RST, but since
		// we've had no chance to close cleanly and the socket must
		// be destroyed immediately, we've got no choice
		shutdown(m_pInternal->m_fd, SHUT_RDWR);
	}
	// Destroy the socket
	closesocket(m_pInternal->m_fd);
	// Deallocate internal pointer
	delete m_pInternal;
}

void *WaitLoopThreadMain(void *)
{
	CSocketBase::RunWaitLoop();
	return NULL;
}

PS_RESULT CSocketBase::Initialize(ESocketProtocol proto)
{
	ONCE(
		pthread_create(&g_SocketSetInternal.m_Thread, NULL, WaitLoopThreadMain, NULL);
	);

	int res=socket(proto, SOCK_STREAM, 0);

	printf("CSocketBase::Initialize(): socket() res: %d\n", res);

	if (res == -1)
	{
		return INVALID_PROTOCOL;
	}
	
	m_pInternal->m_fd=res;
	m_Proto=proto;

	SetNonBlocking(true);

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
		m_State=SS_UNCONNECTED;
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
#ifdef _WIN32
	unsigned long nb=nonblocking;
	int res=ioctlsocket(m_pInternal->m_fd, FIONBIO, &nb);
	if (res == -1)
		printf("SetNonBlocking: res %d\n", res);
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
	m_NonBlocking=nonblocking;
}

void CSocketBase::SetTcpNoDelay(bool tcpNoDelay)
{
	// Disable Nagle's Algorithm
	int data=tcpNoDelay;
	setsockopt(m_pInternal->m_fd, SOL_SOCKET, TCP_NODELAY, (const char *)&data, sizeof(data));
}

PS_RESULT CSocketBase::Read(void *buf, uint len, uint *bytesRead)
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
			printf("Read error %s [%d]\n", errbuf, error);
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
	return PS_OK;
}

PS_RESULT CSocketBase::Write(void *buf, uint len, uint *bytesWritten)
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
			printf("Write error %s [%d]\n", errbuf, err);
			m_State=SS_UNCONNECTED;
			return CONNECTION_BROKEN;
		}
	}

	*bytesWritten=res;
	return PS_OK;
}

PS_RESULT CSocketBase::Connect(const CSocketAddress &addr)
{
	int res=connect(m_pInternal->m_fd, (struct sockaddr *)&addr, sizeof(addr));

	if (res != 0)
	{
		int error=Network_LastError;
		if (m_NonBlocking && error == EWOULDBLOCK)
			m_State=SS_CONNECT_STARTED;
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

	res=bind(m_pInternal->m_fd, (struct sockaddr *)&address, sizeof(address));
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
				printf("CServerSocket::Bind(): bind: %s [%d]\n", errBuf, err);
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
		printf("CServerSocket::Bind(): listen: %s [%d]\n", errBuf, err);
		m_State=SS_UNCONNECTED;
		return PS_FAIL;
	}

	m_State=SS_CONNECTED;
	m_Error=PS_OK;
	return PS_OK;
}

PS_RESULT CSocketBase::PreAccept(CSocketAddress &addr)
{
	socklen_t addrLen=sizeof(addr.m_Union);
	int fd=accept(m_pInternal->m_fd, (struct sockaddr *)&addr.m_Union, &addrLen);
	m_pInternal->m_AcceptFd=fd;
	m_pInternal->m_AcceptAddr=addr;
	if (fd != -1)
		return PS_OK;
	else
		return PS_FAIL;
}

CSocketInternal *CSocketBase::Accept()
{
	if (m_pInternal->m_AcceptFd != -1)
	{
		CSocketInternal *pInt=new CSocketInternal();
		pInt->m_fd=m_pInternal->m_AcceptFd;
		pInt->m_RemoteAddr=m_pInternal->m_AcceptAddr;
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
#ifndef _WIN32
// ConnectError is called on a socket the first time it selects as ready
// after the BeginConnect, to check errors on the socket and update the
// connection status information
// Returns: true if error callback should be called, false if it should not
bool ConnectError(CSocketBase *pSocket, CSocketInternal *pInt)
{
	uint buf;
	int res;
	PS_RESULT connErr;
	
	if (pSocket->m_State==SS_CONNECT_STARTED)
	{
		res=read(pInt->m_fd, &buf, 0);
		// read of zero bytes should be a successful no-op, unless
		// there was an error
		if (res == -1)
		{
			pSocket->m_State=SS_UNCONNECTED;
			PS_RESULT connErr=GetPS_RESULT(errno);
			printf("Connect error: %s [%d:%s]\n", connErr, errno, strerror(errno));
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

void CSocketBase::RunWaitLoop()
{
	int res;

	signal(SIGPIPE, SIG_IGN);

	pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);

	// Create Control Pipe
	res=pipe(g_SocketSetInternal.m_Pipe);
	if (res != 0)
	{
		g_SocketSetInternal.m_Pipe[0] == -1;
		return;
	}
	
	if (g_SocketSetInternal.m_Pipe[0] == -1)
	{
		pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);
		return;
	}
	
	while (true)
	{
		
		std::map<int, CSocketBase *>::iterator it;
		fd_set rfds;
		fd_set wfds;
		int fd_max=g_SocketSetInternal.m_Pipe[0];

		// Prepare fd_set: Read + Control Pipe
		FD_ZERO(&rfds);
		FD_SET(fd_max, &rfds);
		// Prepare fd_set: Write
		FD_ZERO(&wfds);

		it=g_SocketSetInternal.m_HandleMap.begin();
		while (it != g_SocketSetInternal.m_HandleMap.end())
		{
			//printf("Pre select: fd %d has %d\n", it->first, it->second->m_pInternal->m_Ops);
	
			uint ops=it->second->m_pInternal->m_Ops;

			if (ops && it->first > fd_max)
				fd_max=it->first;
			if (ops & READ)
				FD_SET(it->first, &rfds);
			if (ops & WRITE)
				FD_SET(it->first, &wfds);

			++it;
		}
		
		pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);

		//printf("Pre select: fd_max is %d\n", fd_max);

		// select, timeout infinite
		res=select(fd_max+1, &rfds, &wfds, NULL, NULL);

		//printf("Post select: res is %d\n", res);

		// Check select error
		if (res == -1)
		{
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
					// Way out is here, and no locks are held
					return;
				else if (bt=='r')
				{
					//printf("Op mask reload after select\n");
					continue;
				}
			}
			
			FD_CLR(g_SocketSetInternal.m_Pipe[0], &rfds);
		}

		pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);

		// Go through sockets
		int i=-1;
		while (++i <= fd_max)
		{
			//printf("Trying socket %d\n", it->first);
			
			if (!FD_ISSET(i, &rfds) && !FD_ISSET(i, &wfds))
				continue;
			
			it=g_SocketSetInternal.m_HandleMap.find(i);
			if (it == g_SocketSetInternal.m_HandleMap.end())
				continue;
			
			CSocketBase *pSock=it->second;
			CSocketInternal *pInt=pSock->m_pInternal;
			
			if (FD_ISSET(i, &wfds))
			{
				bool callWrite=true;
				
				if (pSock->m_State != SS_CONNECTED)
					callWrite=!ConnectError(pSock, pInt);
				
				pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);
				
				if (callWrite)
					pSock->OnWrite();
				else
					pSock->OnClose(pSock->m_Error);
	
				pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);
			}

			// After the callback is called, we must check if the socket
			// still exists
			it=g_SocketSetInternal.m_HandleMap.find(i);
			if (it == g_SocketSetInternal.m_HandleMap.end())
				continue;

			if (FD_ISSET(i, &rfds))
			{
				bool callRead;
				
				if (pSock->m_State == SS_CONNECT_STARTED)
					callRead=!ConnectError(pSock, pInt);
				else if (pSock->m_State == SS_CONNECTED)
				{
					uint nRead;
					errno=0;
					res=ioctl(i, FIONREAD, &nRead);
					// failure, errno=EINVAL means server socket
					// success, nRead!=0 means alive stream socket
					if ((res == -1 && errno != EINVAL) ||
						(res == 0 && nRead == 0))
					{
						printf("RunWaitLoop:ioctl: Connection broken [%d:%s]\n", errno, strerror(errno));
						pSock->m_State=SS_UNCONNECTED;
						if (errno)
							pSock->m_Error=GetPS_RESULT(errno);
						else
							pSock->m_Error=PS_OK;
						callRead=false;
					}
					else
						callRead=true;
				}
				else
					// UNCONNECTED sockets don't get callbacks
					// Note that server sockets that are bound have state==SS_CONNECTED
					continue;
				
				pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);
			
				if (callRead)
					pSock->OnRead();
				else
					pSock->OnClose(pSock->m_Error);
			
				pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);
			}
		}
	}

	return;
}

void CSocketBase::SendWaitLoopAbort()
{
	char msg='q';
	write(g_SocketSetInternal.m_Pipe[1], &msg, 1);
}

void CSocketBase::SendWaitLoopUpdate()
{
	printf("SendWaitLoopUpdate: fd %d, ops %u\n", m_pInternal->m_fd, m_pInternal->m_Ops);
	char msg='r';
	write(g_SocketSetInternal.m_Pipe[1], &msg, 1);
}

#endif
// Windows WindowProc for async event notification
#ifdef _WIN32



void WaitLoop_SocketUpdateProc(int fd, int error, uint event)
{
	pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);
	CSocketBase *pSock=g_SocketSetInternal.m_HandleMap[fd];
	pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);

	if (error)
	{
		PS_RESULT res=GetPS_RESULT(error);
		if (res == PS_FAIL)
			pSock->OnClose(CONNECTION_BROKEN);
		pSock->m_Error=res;
		pSock->m_State=SS_UNCONNECTED;
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

void CSocketBase::RunWaitLoop()
{
	int ret;
	char errBuf[256];
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
		Network_GetErrorString(ret, errBuf, sizeof(errBuf));
		printf("RegisterClass: %s [%d]\n", errBuf, ret);
	}

	pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);
	// Create message window
	g_SocketSetInternal.m_hWnd=CreateWindow((LPCTSTR)atom, "Network Event Window", WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	if (!g_SocketSetInternal.m_hWnd)
	{
		ret=GetLastError();
		Network_GetErrorString(ret, errBuf, sizeof(errBuf));
		printf("CreateWindowEx: %s [%d]\n", errBuf, ret);
	}
	//pthread_cond_signal(&g_SocketSetInternal.m_CondVar);
	pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);

	if (!g_SocketSetInternal.m_hWnd)
	{
		//TODO Some kind of error message, and exit
		return;
	}
	
	pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);
	
	// If OpMasks where set in another thread before we got this far,
	// WSAAsyncSelect will need to be called again
	std::map<int, CSocketBase *>::iterator it;
	it=g_SocketSetInternal.m_HandleMap.begin();
	while (it != g_SocketSetInternal.m_HandleMap.end())
	{
		it->second->SetOpMask(it->second->GetOpMask());
		++it;
	}

	pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);
	
	printf("Commencing message loop. hWnd %p\n", g_SocketSetInternal.m_hWnd);
	while ((ret=GetMessage(&msg, g_SocketSetInternal.m_hWnd, 0, 0))!=0)
	{
		//printf("RunWaitLoop(): Windows message: %d:%d:%d\n", msg.message, msg.wParam, msg.lParam);
		if (ret == -1)
		{
			ret=GetLastError();
			Network_GetErrorString(ret, errBuf, sizeof(errBuf));
			printf("GetMessage: %s [%d]\n", errBuf, ret);
		}
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	//TODO Destroy window, reset m_hWnd

	printf("RunWaitLoop returning\n");
	return;
}

void CSocketBase::SendWaitLoopAbort()
{
	if (g_SocketSetInternal.m_hWnd)
	{
		PostMessage(g_SocketSetInternal.m_hWnd, WM_QUIT, 0, 0);
	}
	else
		printf("SendWaitLoopUpdate: No WaitLoop Running.\n");
}

void CSocketBase::SendWaitLoopUpdate()
{
	pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);
	if (g_SocketSetInternal.m_hWnd)
	{
		long wsaOps=FD_CLOSE;
		if (m_pInternal->m_Ops & READ)
			wsaOps |= FD_READ|FD_ACCEPT;
		if (m_pInternal->m_Ops & WRITE)
			wsaOps |= FD_WRITE|FD_CONNECT;
		pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);
		//printf("SendWaitLoopUpdate: %d: %u %x -> %p\n", m_pInternal->m_fd, m_pInternal->m_Ops, wsaOps, g_SocketSetInternal.m_hWnd);
		WSAAsyncSelect(m_pInternal->m_fd, g_SocketSetInternal.m_hWnd, MSG_SOCKET_READY, wsaOps);
	}
	else
	{
		//printf("SendWaitLoopUpdate: No WaitLoop Running.\n");
		pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);
	}
}
#endif

void CSocketBase::AbortWaitLoop()
{
	pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);
	SendWaitLoopAbort();
	pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);
//	pthread_join(g_SocketSetInternal.m_Thread);
}

uint CSocketBase::GetOpMask()
{
	return m_pInternal->m_Ops;
}

void CSocketBase::SetOpMask(uint ops)
{
	pthread_mutex_lock(&g_SocketSetInternal.m_Mutex);
	g_SocketSetInternal.m_HandleMap[m_pInternal->m_fd]=this;
	m_pInternal->m_Ops=ops;
	
	/*printf("SetOpMask(fd %d, ops %u) %u\n",
		m_pInternal->m_fd,
		ops,
		g_SocketSetInternal.m_HandleMap[m_pInternal->m_fd]->m_pInternal->m_Ops);*/

	pthread_mutex_unlock(&g_SocketSetInternal.m_Mutex);
	
	SendWaitLoopUpdate();
}
