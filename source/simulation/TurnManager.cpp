#include "precompiled.h"

#include "TurnManager.h"
#include "Network/NetMessage.h"
#include "Network/Network.h"
#include "GameRecord.h"
#include "CLogger.h"

#include <vector>

CSinglePlayerTurnManager *g_SinglePlayerTurnManager=NULL;

CTurnManager::CTurnManager()
{
	for (int i=0;i<3;i++)
		m_Batches[i].m_TurnLength=40;
}

void CTurnManager::ClearBatch(uint batch)
{
	typedef std::vector<SMessageSyncEntry> MsgVector;
	MsgVector &messages=m_Batches[batch].m_Messages;
	MsgVector::iterator it=messages.begin();
	while (it != messages.end())
	{
		delete it->m_pMessage;
		++it;
	}
	messages.clear();
}

void CTurnManager::SBatch::Swap(SBatch &other)
{
	std::swap(m_Messages, other.m_Messages);
	std::swap(m_TurnLength, other.m_TurnLength);
}

void CTurnManager::RotateBatches()
{
	// {a, b, c} => {b, c, a}:
		// {a, b, c}
		// -- swap (0, 1)
		// {b, a, c}
		// -- swap (1, 2)
		// {b, c, a}

	m_Batches[0].Swap(m_Batches[1]);
	m_Batches[1].Swap(m_Batches[2]);
}

void CTurnManager::IterateBatch(uint batch, BatchIteratorFunc *fp, void *userdata)
{
	typedef std::vector<SMessageSyncEntry> MsgVector;
	MsgVector &messages=m_Batches[batch].m_Messages;
	MsgVector::iterator it=messages.begin();
	while (it != messages.end())
	{
		it->m_ClientMask=(*fp)(it->m_pMessage, it->m_ClientMask, userdata);
		++it;
	}
}

void CTurnManager::SendBatch(uint batch)
{
	typedef std::vector<SMessageSyncEntry> MsgVector;
	MsgVector &messages=m_Batches[batch].m_Messages;
	MsgVector::iterator it=messages.begin();
	while (it != messages.end())
	{
		SendMessage(it->m_pMessage, it->m_ClientMask);
		++it;
	}
	SendMessage(new CEndCommandBatch(), (uint)-1);
}

void CTurnManager::SendMessage(CNetMessage *pMsg, uint clientMask)
{
	for (uint i=0;i<m_Clients.size();i++)
	{
		if (clientMask & (1<<i))
		{
			if (m_Clients[i].m_Pipe)
				m_Clients[i].m_Pipe->Push(pMsg->Copy());
		}
	}
}

void CTurnManager::QueueMessage(uint batch, CNetMessage *pMsg)
{
	m_Batches[batch].m_Messages.push_back(SMessageSyncEntry(pMsg));
}

void CTurnManager::SetClientPipe(uint client, IMessagePipeEnd *pipe)
{
	m_Clients[client].m_Pipe=pipe;
}

uint CTurnManager::GetTurnLength()
{
	return m_Batches[0].m_TurnLength;
}

void CTurnManager::Initialize(size_t numClients)
{
	m_Clients.resize(numClients);
}


void CTurnManager::RecordBatch(uint batch)
{
	IterateBatch(batch, RecordIterator, m_pRecord);
	CEndCommandBatch msg;
	m_pRecord->WriteMessage(&msg);
}

uint CTurnManager::RecordIterator(CNetMessage *pMsg, uint clientMask, void *userdata)
{
	CGameRecord *pRecord=(CGameRecord *)userdata;

	pRecord->WriteMessage(pMsg);

	return clientMask;
}

CSinglePlayerTurnManager::CSinglePlayerTurnManager()
{}

void CSinglePlayerTurnManager::NewTurn()
{
	RecordBatch(2);

	RotateBatches();
	ClearBatch(2);
}

void CSinglePlayerTurnManager::QueueLocalCommand(CNetMessage *pMsg)
{
	QueueMessage(2, pMsg);
}
