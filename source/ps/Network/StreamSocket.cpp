#include "Network.h"
#include "StreamSocket.h"

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
	SocketAddress addr;

	res=SocketAddress::Resolve(pSock->m_pConnectHost, pSock->m_ConnectPort, addr);
	if (res == PS_OK)
	{
		pSock->Initialize();
		pSock->SetNonBlocking(false);
		res=pSock->Connect(addr);
	}
	
	pSock->SetNonBlocking(true);
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
	
	OnRead();
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
	m_WriteContext.m_Valid=true;
	m_WriteContext.m_pBuffer=buf;
	m_WriteContext.m_Length=len;
	m_WriteContext.m_Completed=0;
	
	OnWrite();
	SetOpMask(GetOpMask()|WRITE);
	
	return PS_OK;
}

void CStreamSocket::Close()
{
	//TODO Define
}

/*PS_RESULT CStreamSocket::GetRemoteAddress(u8 (&address)[4], int &port)
{
	PS_RESULT res=GetStatus();

	if (res == PS_OK)
		CServerSocket::GetRemoteAddress(m_pInternal, address, port);

	return res;
}*/

#define MakeDefaultCallback(_nm) void CStreamSocket::_nm(PS_RESULT error) \
	{ printf("CStreamSocket::"#_nm"(): %s\n", error); }

void CStreamSocket::OnClose(PS_RESULT error)
{
	printf("CStreamSocket::OnClose(): %s\n", error);
}

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
	PS_RESULT res=CSocketBase::Write(((char *)m_WriteContext.m_pBuffer)+m_WriteContext.m_Completed, m_WriteContext.m_Length-m_WriteContext.m_Completed, &bytes);
	if (res != PS_OK)
	{
		WriteComplete(res);
		return;
	}
	printf("OnWrite(): %u bytes\n", bytes);
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
		SetOpMask(GetOpMask() & (~READ));
		return;
	}
	uint bytes=0;
	PS_RESULT res=CSocketBase::Read(
		((char *)m_ReadContext.m_pBuffer)+m_ReadContext.m_Completed,
		m_ReadContext.m_Length-m_ReadContext.m_Completed,
		&bytes);
	if (res != PS_OK)
	{
		ReadComplete(res);
		return;
	}
	printf("OnRead(): %u bytes read of %u\n", bytes, m_ReadContext.m_Length-m_ReadContext.m_Completed);
	m_ReadContext.m_Completed+=bytes;
	if (m_ReadContext.m_Completed == m_ReadContext.m_Length)
	{
		m_ReadContext.m_Valid=false;
		ReadComplete(PS_OK);
	}
}
