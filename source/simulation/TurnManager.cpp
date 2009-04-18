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

#include "TurnManager.h"
#include "network/NetMessage.h"
#include "network/Network.h"
#include "ps/GameRecord.h"
#include "ps/CLogger.h"

#include <vector>

CSinglePlayerTurnManager *g_SinglePlayerTurnManager=NULL;

CTurnManager::CTurnManager()
{
	for (int i=0; i<3; i++)
		m_Batches[i].m_TurnLength = DEFAULT_TURN_LENGTH;
}

void CTurnManager::ClearBatch(uintptr_t batch)
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

void CTurnManager::IterateBatch(uintptr_t batch, BatchIteratorFunc *fp, void *userdata)
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

void CTurnManager::SendBatch(uintptr_t batch)
{
	typedef std::vector<SMessageSyncEntry> MsgVector;
	MsgVector &messages=m_Batches[batch].m_Messages;
	MsgVector::iterator it=messages.begin();
	while (it != messages.end())
	{
		SendMessage(it->m_pMessage, it->m_ClientMask);
		++it;
	}
	CEndCommandBatchMessage *pMsg=new CEndCommandBatchMessage();
	pMsg->m_TurnLength=m_Batches[batch].m_TurnLength;
	SendMessage(pMsg, (size_t)-1);
}

void CTurnManager::SendMessage(CNetMessage *pMsg, size_t clientMask)
{
	for (size_t i=0;i<m_Clients.size();i++)
	{
		if (clientMask & (1<<i))
		{
			if (m_Clients[i].m_Pipe)
				m_Clients[i].m_Pipe->Push(pMsg);
		}
	}
}

void CTurnManager::QueueMessage(uintptr_t batch, CNetMessage *pMsg)
{
	m_Batches[batch].m_Messages.push_back(SMessageSyncEntry(pMsg));
}

void CTurnManager::SetClientPipe(size_t client, IMessagePipeEnd *pipe)
{
	m_Clients[client].m_Pipe=pipe;
}

void CTurnManager::SetTurnLength(uintptr_t batch, int turnLength)
{
	m_Batches[batch].m_TurnLength=turnLength;
}

int CTurnManager::GetTurnLength()
{
	return m_Batches[0].m_TurnLength;
}

void CTurnManager::Initialize(size_t numClients)
{
	m_Clients.resize(numClients);
}


void CTurnManager::RecordBatch(uintptr_t batch)
{
	IterateBatch(batch, RecordIterator, m_pRecord);
	CEndCommandBatchMessage msg;
	m_pRecord->WriteMessage(&msg);
}

size_t CTurnManager::RecordIterator(CNetMessage *pMsg, uintptr_t clientMask, void *userdata)
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

bool CSinglePlayerTurnManager::NewTurnReady()
{
	return true;
}
