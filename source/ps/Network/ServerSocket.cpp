#include "Network.h"

CServerSocket::~CServerSocket()
{
	// We must ensure that the CSocket destructor doesn't try to
	// disconnect the server socket
	//FIXME stuff
}
	
/*void CServerSocket::GetRemoteAddress(CSocketInternal *pInt, u8 (&address)[4], int &port)
{
	port=ntohs(pInt->m_RemoteAddr.sin_port);
	address[0]=(u8)(pInt->m_RemoteAddr.sin_addr.s_addr & 0xff);
	address[1]=(u8)((pInt->m_RemoteAddr.sin_addr.s_addr >> 8) & 0xff);
	address[2]=(u8)((pInt->m_RemoteAddr.sin_addr.s_addr >> 16) & 0xff);
	address[3]=(u8)(pInt->m_RemoteAddr.sin_addr.s_addr >> 24);
}*/

void CServerSocket::OnRead()
{
	SocketAddress remoteAddr;
	
	PS_RESULT res=PreAccept(remoteAddr);
	if (res==PS_OK)
	{
		OnAccept(remoteAddr);
	}
	// All errors are non-critical, so no need to do anything special besides
	// not calling OnAccept
}

void CServerSocket::OnWrite()
{}

void CServerSocket::OnClose(PS_RESULT errorCode)
{}
