#include "precompiled.h"

#include "TurnManager.h"
#include "Network/NetMessage.h"
#include "Network/Network.h"
#include "GameRecord.h"

#include <vector>

CSinglePlayerTurnManager *g_SinglePlayerTurnManager=NULL;

CTurnManager::CTurnManager()
{
	for (int i=0;i<3;i++)
		m_Batches[i].m_TurnLength=500;
}

void CTurnManager::ClearBatch(uint batch)
{
	m_Batches[batch].m_Messages.clear();
}

void CTurnManager::SBatch::Swap(SBatch &other)
{
	std::swap(m_Messages, other.m_Messages);
	std::swap(m_TurnLength, other.m_TurnLength);
}

void CTurnManager::RotateBatches()
{
	// {0, 1, 2} => {1, 2, 0}:
		// {0, 1, 2}
		// -- swap (0, 1)
		// {1, 0, 2}
		// -- swap (1, 2)
		// {1, 2, 0}

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
		(*fp)(it->m_pMessage, it->m_ClientMask, userdata);
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
		it=messages.erase(it);
	}
}

void CTurnManager::SendMessage(CNetMessage *pMsg, uint clientMask)
{
	uint sendToCount=0;
	for (uint i=0;i<m_Clients.size();i++)
	{
		if (clientMask & (1<<i))
			sendToCount++;
	}
	for (uint i=0;i<m_Clients.size();i++)
	{
		if (clientMask & (1<<i))
		{
			CNetMessage *sendMsg;
			if (--sendToCount)
				sendMsg=pMsg->Copy();
			else // Last message - send original message instead of copying it
				sendMsg=pMsg;
			m_Clients[i].m_Pipe->Push(sendMsg);
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
