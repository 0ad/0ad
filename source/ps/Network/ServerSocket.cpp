#include "precompiled.h"

#include "Network.h"
#include "CLogger.h"

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
	else
	{
		// All errors are non-critical, so no need to do anything special besides
		// not calling OnAccept [ shouldn't be, that is ;-) ]
		LOG(NORMAL, LOG_CAT_NET, "CServerSocket::OnRead(): PreAccept returned an error: %s", res);
	}
}

void CServerSocket::OnWrite()
{}

void CServerSocket::OnClose(PS_RESULT errorCode)
{}
