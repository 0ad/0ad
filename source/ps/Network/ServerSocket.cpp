#include "precompiled.h"

#include "Network.h"

CServerSocket::~CServerSocket()
{
}
	
void CServerSocket::OnRead()
{
	CSocketAddress remoteAddr;
	
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
