#include "precompiled.h"

#include "Network.h"
#include "StreamSocket.h"

#include <CLogger.h>

CStreamSocket::CStreamSocket()
{}

CStreamSocket::CStreamSocket(CSocketInternal *pInt):
	CSocketBase(pInt)
{}

CStreamSocket::~CStreamSocket()
{
}

void *CStreamSocket_ConnectThread(void *data)
{
	CStreamSocket *pSock=(CStreamSocket *)data;
	PS_RESULT res=PS_OK;
	CSocketAddress addr;

	res=CSocketAddress::Resolve(pSock->m_pConnectHost, pSock->m_ConnectPort, addr);
	LOG(NORMAL, LOG_CAT_NET, "CStreamSocket_ConnectThread: Resolve: %s -> %s", res, addr.GetString().c_str());
	if (res == PS_OK)
	{
		pSock->Initialize();
		pSock->SetNonBlocking(false);
		res=pSock->Connect(addr);
		if (res != PS_OK)
			LOG(ERROR, LOG_CAT_NET, "CStreamSocket_ConnectThread: Connect: %s", res);
	}
	
	if (res == PS_OK)
	{
		pSock->SetNonBlocking(true);
	
		// This should call the right callbacks, so that you get the expected
		// results if you call Read or Write before the connect actually is complete
		pSock->SetOpMask((pSock->m_WriteContext.m_Valid?CSocketBase::WRITE:0)|CSocketBase::READ);
	}
	
	pSock->ConnectComplete(res);

	free(pSock->m_pConnectHost);

	return NULL;
}

PS_RESULT CStreamSocket::BeginConnect(const char *hostname, int port)
{
	m_pConnectHost=strdup(hostname);
	m_ConnectPort=port;

	// Start thread
	pthread_t thread;
	pthread_create(&thread, NULL, &CStreamSocket_ConnectThread, this);

	return PS_OK;
}

PS_RESULT CStreamSocket::Read(void *buf, uint len)
{
	// Check socket status
	if (GetState() != SS_CONNECTED)
		return GetErrorState();

	// Check for running read operation
	if (m_ReadContext.m_Valid)
		return CONFLICTING_OP_IN_PROGRESS;

	// Fill in read_cb
	m_ReadContext.m_Valid=true;
	m_ReadContext.m_pBuffer=buf;
	m_ReadContext.m_Length=len;
	m_ReadContext.m_Completed=0;
	
	SetOpMask(GetOpMask()|READ);
	
	return PS_OK;
}

PS_RESULT CStreamSocket::Write(void *buf, uint len)
{
	// Check status
	if (GetState() != SS_CONNECTED)
		return GetErrorState();
	
	// Check running Write operation
	if (m_WriteContext.m_Valid)
		return CONFLICTING_OP_IN_PROGRESS;

	// Fill in read_cb
	m_WriteContext.m_pBuffer=buf;
	m_WriteContext.m_Length=len;
	m_WriteContext.m_Completed=0;
	m_WriteContext.m_Valid=true;
	
	SetOpMask(GetOpMask()|WRITE);
	
	return PS_OK;
}

#define MakeDefaultCallback(_nm) void CStreamSocket::_nm(PS_RESULT error) \
	{ printf("CStreamSocket::"#_nm"(): %s\n", error); }

MakeDefaultCallback(OnClose)
MakeDefaultCallback(ConnectComplete)
MakeDefaultCallback(ReadComplete)
MakeDefaultCallback(WriteComplete)

void CStreamSocket::OnWrite()
{
	if (!m_WriteContext.m_Valid)
	{
		SetOpMask(GetOpMask() & (~WRITE));
		return;
	}
	uint bytes=0;
	PS_RESULT res=CSocketBase::Write(
		((char *)m_WriteContext.m_pBuffer)+m_WriteContext.m_Completed,
		m_WriteContext.m_Length-m_WriteContext.m_Completed,
		&bytes);
	if (res != PS_OK)
	{
		WriteComplete(res);
		return;
	}
	printf("CStreamSocket::OnWrite(): %u bytes\n", bytes);
	m_WriteContext.m_Completed+=bytes;
	if (m_WriteContext.m_Completed == m_WriteContext.m_Length)
	{
		m_WriteContext.m_Valid=false;
		WriteComplete(PS_OK);
	}
}

void CStreamSocket::OnRead()
{
	if (!m_ReadContext.m_Valid)
	{
		printf("CStreamSocket::OnRead(): No Read request in progress\n");
		//SetOpMask(GetOpMask() & (~READ));
		return;
	}
	uint bytes=0;
	PS_RESULT res=CSocketBase::Read(
		((char *)m_ReadContext.m_pBuffer)+m_ReadContext.m_Completed,
		m_ReadContext.m_Length-m_ReadContext.m_Completed,
		&bytes);
	printf("CStreamSocket::OnRead(): %s, %u bytes read of %u\n", res, bytes,
		m_ReadContext.m_Length-m_ReadContext.m_Completed);
	if (res != PS_OK)
	{
		ReadComplete(res);
		return;
	}
	m_ReadContext.m_Completed+=bytes;
	if (m_ReadContext.m_Completed == m_ReadContext.m_Length)
	{
		m_ReadContext.m_Valid=false;
		ReadComplete(PS_OK);
	}
}
