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
#include "Serialization.h"
#include "ps/CLogger.h"
#include "NetLog.h"


DEFINE_ERROR(CONFLICTING_OP_IN_PROGRESS, "A conflicting operation is already in progress");

#define LOG_CATEGORY L"net"

/**
 * The SNetHeader will always be stored in host-order
 */
struct SNetHeader
{
	u8 m_MsgType;
	u16 m_MsgLength;

	inline const u8 *Deserialize(const u8 *pos)
	{
		Deserialize_int_1(pos, m_MsgType);
		Deserialize_int_2(pos, m_MsgLength);
		return pos;
	}

	inline u8 *Serialize(u8 *pos) const
	{
		Serialize_int_1(pos, m_MsgType);
		Serialize_int_2(pos, m_MsgLength);
		return pos;
	}
};
#define HEADER_LENGTH 3

CMessagePipe::CMessagePipe()
{
	m_Ends[0]=End(this, &m_Queues[0], &m_Queues[1]);
	m_Ends[1]=End(this, &m_Queues[1], &m_Queues[0]);
//	pthread_cond_init(&m_CondVar, NULL);
}

void CMessagePipe::End::Push(CNetMessage *msg)
{
	m_pOut->Lock();
	m_pOut->push_back(msg);
	m_pOut->Unlock();
	/*pthread_mutex_lock(&m_pPipe->m_CondMutex);
	pthread_cond_broadcast(&m_pPipe->m_CondVar);
	pthread_mutex_unlock(&m_pPipe->m_CondMutex);*/
}

CNetMessage *CMessagePipe::End::TryPop()
{
	CScopeLock lock(m_pIn->m_Mutex);
	if (m_pIn->size())
	{
		CNetMessage *msg=m_pIn->front();
		m_pIn->pop_front();
		return msg;
	}
	return NULL;
}

/*void CMessagePipe::End::WaitPop(CNetMessage *msg)
{
	while (!TryPop(msg))
	{
		pthread_mutex_lock(&m_pPipe->m_CondMutex);
		pthread_cond_wait(&m_pPipe->m_CondVar, &m_pPipe->m_CondMutex);
		pthread_mutex_unlock(&m_pPipe->m_CondMutex);
	}
}*/

/*CStr CErrorMessage::GetString() const
{
	static const char* const states[]={
		"SS_UNCONNECTED",
		"SS_CONNECT_STARTED",
		"SS_CONNECTED",
		"SS_CLOSED_LOCALLY"
	};

	return CStr("NetErrorMessage: ")+
		m_Error+", Socket State "+states[m_State];
}

CStr CConnectCompleteMessage::GetString() const
{
	return CStr("ConnectCompleteMessage");
}

CStr CCloseRequestMessage::GetString() const
{
	return CStr("CloseRequestMessage");
}*/

CStr CErrorMessage::ToString( void ) const
{
	static const char* const states[]=
	{
		"SS_UNCONNECTED",
		"SS_CONNECT_STARTED",
		"SS_CONNECTED",
		"SS_CLOSED_LOCALLY"
	};

	return CStr("NetErrorMessage: ")+
		m_Error+", Socket State "+states[m_State];
}

CStr CConnectCompleteMessage::ToString( void ) const
{
	return CStr( "ConnectCompleteMessage" );
}

CStr CCloseRequestMessage::ToString( void ) const
{
	return CStr( "CloseRequestMessage" );
}

void CMessageSocket::Push(CNetMessage *msg)
{
	NET_LOG2( "CMessageSocket::Push(): %hs", msg->ToString().c_str() );

	m_OutQ.Lock();
	m_OutQ.push_back(msg);
	m_OutQ.Unlock();
	StartWriteNextMessage();
}

CNetMessage *CMessageSocket::TryPop()
{
	CScopeLock lock(m_InQ.m_Mutex);
	if (m_InQ.size())
	{
		CNetMessage *msg=m_InQ.front();
		m_InQ.pop_front();
		return msg;
	}
	return NULL;
}


void CMessageSocket::StartWriteNextMessage()
{
	m_OutQ.Lock();
	if (!m_IsWriting && m_OutQ.size())
	{
		CNetMessage *pMsg=NULL;
		while (pMsg == NULL)
		{
			// This may happen when the last message of the queue is an invalid
			// message type (non-network or socket command)
			if (m_OutQ.size() == 0)
				return;

			// Pop next output message
			pMsg=m_OutQ.front();
			m_OutQ.pop_front();
			m_IsWriting=true;
			m_OutQ.Unlock();
			
			if (pMsg->GetType() == NMT_CLOSE_REQUEST)
			{
				Close();
				delete pMsg;
				pMsg=NULL;
			}
			else if (pMsg->GetType() < 0)
			{
				LOG(CLogger::Warning, LOG_CATEGORY, L"CMessageSocket::StartWriteNextMessage(): Non-network message");
				delete pMsg;
				pMsg=NULL;
			}
		}

		// Prepare the header
		SNetHeader hdr;
		hdr.m_MsgType=pMsg->GetType();
		hdr.m_MsgLength=(u16)pMsg->GetSerializedLength();

		// Allocate buffer space
		if ((size_t)(hdr.m_MsgLength+HEADER_LENGTH) > m_WrBufferSize)
		{
			//m_WrBufferSize = BUFFER_SIZE(hdr.m_MsgLength+HEADER_LENGTH);
			m_WrBufferSize = ALIGN_BLOCK(hdr.m_MsgLength+HEADER_LENGTH);
			if (m_pWrBuffer)
				m_pWrBuffer=(u8 *)realloc(m_pWrBuffer, m_WrBufferSize);
			else
				m_pWrBuffer=(u8 *)malloc(m_WrBufferSize);
		}

		// Fill in buffer
		u8 *pos=m_pWrBuffer;
		pos=hdr.Serialize(pos);
		pMsg->Serialize(pos);

		// Deallocate message
		delete pMsg;

		// Start Write Operation
		//printf("CMessageSocket::StartWriteNextMessage(): Writing an MT %d, length %u (%u)\n", hdr.m_MsgType, hdr.m_MsgLength+HEADER_LENGTH, hdr.m_MsgLength);
		PS_RESULT res=Write(m_pWrBuffer, hdr.m_MsgLength+HEADER_LENGTH);
		if (res != PS_OK)
		{
			NET_LOG2( "CMessageSocket::StartWriteNextMessage(): %hs", res );

			// Queue Error Message
			m_InQ.Lock();
			m_InQ.push_back(new CErrorMessage(res, GetState()));
			m_InQ.Unlock();
		}
	}
	else
	{
		if (m_IsWriting)
		{
			NET_LOG( "CMessageSocket::StartWriteNextMessage(): Already writing" );
		}
		else
		{
			NET_LOG("CMessageSocket::StartWriteNextMessage(): Nothing to write");
		}
		m_OutQ.Unlock();
	}
}

void CMessageSocket::WriteComplete(PS_RESULT ec)
{
	NET_LOG2( "CMessageSocket::WriteComplete(): %hs", ec );

	if (ec == PS_OK)
	{
		if (m_IsWriting)
		{
			m_OutQ.Lock();
			m_IsWriting=false;
			m_OutQ.Unlock();
			StartWriteNextMessage();
		}
		else
		{
			NET_LOG( "CMessageSocket::WriteComplete(): Was not writing" );
		}
	}
	else
	{
		// Push an error message
		m_InQ.push_back(new CErrorMessage(ec, GetState()));
	}
}

void CMessageSocket::StartReadHeader()
{
	if (m_RdBufferSize < HEADER_LENGTH)
	{
		//m_RdBufferSize=BUFFER_SIZE(HEADER_LENGTH);
		m_RdBufferSize=ALIGN_BLOCK(HEADER_LENGTH);
		if (m_pRdBuffer)
			m_pRdBuffer=(u8 *)realloc(m_pRdBuffer, m_RdBufferSize);
		else
			m_pRdBuffer=(u8 *)malloc(m_RdBufferSize);
	}
	m_ReadingData=false;
	printf("CMessageSocket::StartReadHeader(): Trying to read %u\n", HEADER_LENGTH);
	PS_RESULT res=Read(m_pRdBuffer, HEADER_LENGTH);
	if (res != PS_OK)
	{
		NET_LOG2( "CMessageSocket::StartReadHeader(): %hs", res );

		// Push an error message
		CScopeLock scopeLock(m_InQ.m_Mutex);
		m_InQ.push_back(new CErrorMessage(res, GetState()));
	}
}

void CMessageSocket::StartReadMessage()
{
	SNetHeader hdr;
	hdr.Deserialize(m_pRdBuffer);

	size_t reqBufSize=HEADER_LENGTH+hdr.m_MsgLength;
	if (m_RdBufferSize < reqBufSize)
	{
		//m_RdBufferSize=BUFFER_SIZE(reqBufSize);
		m_RdBufferSize=ALIGN_BLOCK(reqBufSize);
		if (m_pRdBuffer)
			m_pRdBuffer=(u8 *)realloc(m_pRdBuffer, m_RdBufferSize);
		else
			m_pRdBuffer=(u8 *)malloc(m_RdBufferSize);
	}
	m_ReadingData=true;
	
	if (hdr.m_MsgLength == 0)
	{
		ReadComplete(PS_OK);
	}
	else
	{
		PS_RESULT res=Read(m_pRdBuffer+HEADER_LENGTH, hdr.m_MsgLength);
		if (res != PS_OK)
		{
			NET_LOG2( "CMessageSocket::StartReadMessage(): %hs", res );

			// Queue an error message
			CScopeLock scopeLock(m_InQ);
			m_InQ.push_back(new CErrorMessage(res, GetState()));
		}
	}
}

void CMessageSocket::ReadComplete(PS_RESULT ec)
{
	NET_LOG3( "CMessageSocket::ReadComplete(%ls): %hs", m_ReadingData ? L"data":L"header", ec );

	// Check if we were reading header or message
	// If header:
	if (!m_ReadingData)
	{
		StartReadMessage();
	}
	// If data:
	else
	{
		SNetHeader hdr;
		hdr.Deserialize(m_pRdBuffer);
		//CNetMessage *pMsg=CNetMessage::DeserializeMessage((ENetMessageType)hdr.m_MsgType, m_pRdBuffer+HEADER_LENGTH, hdr.m_MsgLength);
		CNetMessage *pMsg = CNetMessageFactory::CreateMessage( m_pRdBuffer+HEADER_LENGTH, hdr.m_MsgLength);
		if (pMsg)
		{
			OnMessage(pMsg);
		}
		else
		{
			NET_LOG3( "CMessageSocket::ReadComplete(): Deserialization failed! (type %d, length %d)", hdr.m_MsgType, hdr.m_MsgLength );
			NET_LOG( "Data: {" );

			for (int i=HEADER_LENGTH;i<hdr.m_MsgLength+HEADER_LENGTH;i++)
			{
				NET_LOG3( "\t0x%x [%c]", m_pRdBuffer[i], isalnum(m_pRdBuffer[i])?m_pRdBuffer[i]:'.' );
			}
			NET_LOG( "};" );
		}
		StartReadHeader();
	}
}

void CMessageSocket::OnMessage(CNetMessage *pMsg)
{
	m_InQ.Lock();
	m_InQ.push_back(pMsg);
	NET_LOG2( "CMessageSocket::OnMessage(): %hs", pMsg->ToString().c_str() );
	NET_LOG2( "CMessageSocket::OnMessage(): Queue size now %lu", (unsigned long)m_InQ.size() );
	m_InQ.Unlock();
}

void CMessageSocket::ConnectComplete(PS_RESULT ec)
{
	if (ec == PS_OK)
	{
		StartReadHeader();
		CScopeLock scopeLock(m_InQ);
		m_InQ.push_back(new CConnectCompleteMessage());
	}
	else
	{
		CScopeLock scopeLock(m_InQ);
		m_InQ.push_back(new CErrorMessage(ec, GetState()));
	}
}

void CMessageSocket::OnClose(PS_RESULT errorCode)
{
	CScopeLock scopeLock(m_InQ.m_Mutex);
	m_InQ.push_back(new CErrorMessage(errorCode, GetState()));
}

CMessageSocket::CMessageSocket(CSocketInternal *pInt):
		CStreamSocket(pInt),
		m_IsWriting(false),
		m_pWrBuffer(NULL),
		m_WrBufferSize(0),
		m_ReadingData(false),
		m_pRdBuffer(NULL),
		m_RdBufferSize(0)
{
	StartReadHeader();
}

CMessageSocket::CMessageSocket():
		m_IsWriting(false),
		m_pWrBuffer(NULL),
		m_WrBufferSize(0),
		m_ReadingData(false),
		m_pRdBuffer(NULL),
		m_RdBufferSize(0)
{
}

CMessageSocket::~CMessageSocket()
{
	if (m_pRdBuffer)
		free(m_pRdBuffer);
	if (m_pWrBuffer)
		free(m_pWrBuffer);
}

PS_RESULT CMessageSocket::BeginConnect(const char *address, int port)
{
	StartReadHeader();
	return CStreamSocket::BeginConnect(address, port);
}

// End of Network.cpp
