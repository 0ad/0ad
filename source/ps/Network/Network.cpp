#include "Network.h"
#include "Serialization.h"
#include <errno.h>

DEFINE_ERROR(CONFLICTING_OP_IN_PROGRESS, "A conflicting operation is already in progress");

/**
 * The SNetHeader will always be stored in host-order
 */
struct SNetHeader
{
	u8 m_MsgType;
	u16 m_MsgLength;

	inline void Deserialize(u8 *buf)
	{
		m_MsgType=DeserializeInt<u8, 1>(&buf);
		m_MsgLength=DeserializeInt<u16, 2>(&buf);
	}

	inline u8 *Serialize(u8 *pos)
	{
		pos=SerializeInt<u8, 1>(pos, m_MsgType);
		pos=SerializeInt<u16, 2>(pos, m_MsgLength);
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

void CMessageSocket::Push(CNetMessage *msg)
{
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
		// Pop next output message
		CNetMessage *pMsg=m_OutQ.front();
		m_OutQ.pop_front();
		m_IsWriting=true;
		m_OutQ.Unlock();

		// Prepare the header
		SNetHeader hdr;
		hdr.m_MsgType=pMsg->GetType();
		hdr.m_MsgLength=pMsg->GetSerializedLength();

		// Allocate buffer space
		if (hdr.m_MsgLength+HEADER_LENGTH > m_WrBufferSize)
		{
			m_WrBufferSize = (hdr.m_MsgLength+HEADER_LENGTH);
			m_WrBufferSize += m_WrBufferSize % 256;
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
		printf("StartWriteNextMessage(): Writing an MT %d, length %u\n", hdr.m_MsgType, hdr.m_MsgLength+HEADER_LENGTH);
		PS_RESULT res=Write(m_pWrBuffer, hdr.m_MsgLength+HEADER_LENGTH);
		if (res != PS_OK)
			; // Queue Error Message
	}
	else
	{
		if (m_IsWriting)
			printf("StartWriteNextMessage(): Already writing\n");
		else
			printf("StartWriteNextMessage(): Nothing to write\n");
		m_OutQ.Unlock();
	}
}

void CMessageSocket::WriteComplete(PS_RESULT ec)
{
	printf("WriteComplete(): %s\n", ec);
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
			printf("WriteComplete(): Was not writing\n");
	}
	else
	{
		CScopeLock scopeLock(m_InQ.m_Mutex);
		// Push an error message
	}
}

void CMessageSocket::StartReadHeader()
{
	if (m_RdBufferSize < HEADER_LENGTH)
	{
		m_RdBufferSize=256;
		if (m_pRdBuffer)
			m_pRdBuffer=(u8 *)realloc(m_pRdBuffer, m_RdBufferSize);
		else
			m_pRdBuffer=(u8 *)malloc(m_RdBufferSize);
	}
	m_ReadingData=false;
	printf("StartReadHeader(): Trying to read %u\n", HEADER_LENGTH);
	PS_RESULT res=Read(m_pRdBuffer, HEADER_LENGTH);
	if (res != PS_OK)
		; // Push an error message
}

void CMessageSocket::StartReadMessage()
{
	SNetHeader hdr;
	hdr.Deserialize(m_pRdBuffer);
	uint reqBufSize=HEADER_LENGTH+hdr.m_MsgLength;
	if (m_RdBufferSize < reqBufSize)
	{
		m_RdBufferSize=reqBufSize+(reqBufSize%256);
		if (m_pRdBuffer)
			m_pRdBuffer=(u8 *)realloc(m_pRdBuffer, m_RdBufferSize);
		else
			m_pRdBuffer=(u8 *)malloc(m_RdBufferSize);
	}
	m_ReadingData=true;
	printf("StartReadMessage(): Got type %d, trying to read %u\n", hdr.m_MsgType, hdr.m_MsgLength);
	PS_RESULT res=Read(m_pRdBuffer+HEADER_LENGTH, hdr.m_MsgLength);
	if (res != PS_OK)
		; // Queue an error message
}

void CMessageSocket::ReadComplete(PS_RESULT ec)
{
	printf("ReadComplete(%s): %s\n", m_ReadingData?"true":"false", ec);
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
		CNetMessage *pMsg=CNetMessage::DeserializeMessage((NetMessageType)hdr.m_MsgType, m_pRdBuffer+HEADER_LENGTH, hdr.m_MsgLength);
		if (pMsg)
		{
			m_InQ.Lock();
			m_InQ.push_back(pMsg);
			printf("ReadComplete() has pushed, queue size %u\n", m_InQ.size());
			m_InQ.Unlock();
		}
		StartReadHeader();
	}
}

void CMessageSocket::ConnectComplete(PS_RESULT ec)
{
	StartReadHeader();
}

CMessageSocket::~CMessageSocket()
{
}

// End of Network.cpp