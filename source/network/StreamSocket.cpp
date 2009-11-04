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

#include "Network.h"
#include "StreamSocket.h"

#include "NetLog.h"

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
	debug_SetThreadName("net_connect");

	CStreamSocket *pSock=(CStreamSocket *)data;
	PS_RESULT res=PS_OK;
	CSocketAddress addr;

	res=CSocketAddress::Resolve(pSock->m_pConnectHost, pSock->m_ConnectPort, addr);
	NET_LOG4("CStreamSocket_ConnectThread: Resolve: %hs -> %hs [%hs]", pSock->m_pConnectHost, addr.GetString().c_str(), res);
	if (res == PS_OK)
	{
		pSock->Initialize();
		// If we don't do this we'll get spurious callbacks called since our
		// network thread will notice the socket getting connected (and
		// potentially receiving data) while we might not yet have called the
		// ConnectComplete callback
		pSock->SetOpMask(0);
		pSock->SetNonBlocking(false);
		res=pSock->Connect(addr);
		NET_LOG2("CStreamSocket_ConnectThread: Connect: %hs", res);
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
	pSock->m_pConnectHost=NULL;

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

PS_RESULT CStreamSocket::Read(void *buf, size_t len)
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

PS_RESULT CStreamSocket::Write(void *buf, size_t len)
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
	{ NET_LOG2("CStreamSocket::"#_nm"(): %hs", error); }

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
	size_t bytes=0;
	PS_RESULT res=CSocketBase::Write(
		((char *)m_WriteContext.m_pBuffer)+m_WriteContext.m_Completed,
		m_WriteContext.m_Length-m_WriteContext.m_Completed,
		&bytes);
	if (res != PS_OK)
	{
		WriteComplete(res);
		return;
	}
	NET_LOG2("CStreamSocket::OnWrite(): %lu bytes", (unsigned long)bytes);
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
		NET_LOG("CStreamSocket::OnRead(): No Read request in progress");
		return;
	}
	size_t bytes=0;
	PS_RESULT res=CSocketBase::Read(
		((u8 *)m_ReadContext.m_pBuffer)+m_ReadContext.m_Completed,
		m_ReadContext.m_Length-m_ReadContext.m_Completed,
		&bytes);
	NET_LOG4("CStreamSocket::OnRead(): %hs, %lu bytes read of %lu",
					res, (unsigned long)bytes,
					(unsigned long)(m_ReadContext.m_Length-m_ReadContext.m_Completed));
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
