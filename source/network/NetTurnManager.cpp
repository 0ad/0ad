/* Copyright (C) 2010 Wildfire Games.
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

#include "NetTurnManager.h"

#include "NetServer.h"
#include "NetClient.h"
#include "maths/MathUtil.h"
#include "simulation2/Simulation2.h"

static const int TURN_LENGTH = 200; // TODO: this should be a variable controlled by the server depending on latency

static const int COMMAND_DELAY = 2;

//#define NETTURN_LOG debug_printf

CNetTurnManager::CNetTurnManager(CSimulation2& simulation, int playerId, int clientId) :
	m_Simulation2(simulation), m_CurrentTurn(0), m_ReadyTurn(1), m_DeltaTime(0),
	m_PlayerId(playerId), m_ClientId(clientId)
{
	// When we are on turn n, we schedule new commands for n+2.
	// We know that all other clients have finished scheduling commands for n (else we couldn't have got here).
	// We know we have not yet finished scheduling commands for n+2.
	// Hence other clients can be on turn n-1, n, n+1, and no other.
	// So they can be sending us commands scheduled for n+1, n+2, n+3.
	// So we need a 3-element buffer:
	m_QueuedCommands.resize(COMMAND_DELAY + 1);
}

bool CNetTurnManager::Update(float frameLength)
{
	m_DeltaTime += frameLength;

	// If we haven't reached the next turn yet, do nothing
	if (m_DeltaTime < 0)
		return false;

#ifdef NETTURN_LOG
	NETTURN_LOG(L"Update current=%d ready=%d\n", m_CurrentTurn, m_ReadyTurn);
#endif

	// Check that the next turn is ready for execution
	if (m_ReadyTurn > m_CurrentTurn)
	{
		NotifyFinishedOwnCommands(m_CurrentTurn + COMMAND_DELAY);
		m_CurrentTurn += 1;

		// Put all the client commands into a single list, in a globally consistent order
		std::vector<SimulationCommand> commands;
		for (std::map<u32, std::vector<SimulationCommand> >::iterator it = m_QueuedCommands[0].begin(); it != m_QueuedCommands[0].end(); ++it)
		{
			commands.insert(commands.end(), it->second.begin(), it->second.end());
		}
		m_QueuedCommands.pop_front();
		m_QueuedCommands.resize(m_QueuedCommands.size() + 1);

#ifdef NETTURN_LOG
		NETTURN_LOG(L"Running %d cmds\n", commands.size());
#endif

		m_Simulation2.Update(TURN_LENGTH, commands);

		// TODO: Compute state hash, send SyncCheck message

		// Set the time for the next turn update
		m_DeltaTime -= TURN_LENGTH / 1000.f;

		return true;
	}
	else
	{
		// Oops, we wanted to start the next turn but it's not ready yet -
		// there must be too much network lag.
		// TODO: complain to the user.
		// TODO: send feedback to the server to increase the turn length.

		// Reset the next-turn timer to 0 so we try again next update but
		// so we don't rush to catch up in subsequent turns.
		// TODO: we should do clever rate adjustment instead of just pausing like this.
		m_DeltaTime = 0;

		return false;
	}
}

void CNetTurnManager::Interpolate(float frameLength)
{
	float offset = clamp(m_DeltaTime / (TURN_LENGTH / 1000.f) + 1.0, 0.0, 1.0);
	m_Simulation2.Interpolate(frameLength, offset);
}

void CNetTurnManager::AddCommand(int client, int player, CScriptValRooted data, u32 turn)
{
#ifdef NETTURN_LOG
	NETTURN_LOG(L"AddCommand(client=%d player=%d turn=%d)\n", client, player, turn);
#endif

	if (!(m_CurrentTurn < turn && turn <= m_CurrentTurn + COMMAND_DELAY + 1))
	{
		debug_warn(L"Received command for invalid turn");
		return;
	}

	SimulationCommand cmd;
	cmd.player = player;
	cmd.data = data;
	m_QueuedCommands[turn - (m_CurrentTurn+1)][client].push_back(cmd);
}

void CNetTurnManager::FinishedAllCommands(u32 turn)
{
#ifdef NETTURN_LOG
	NETTURN_LOG(L"FinishedAllCommands(%d)\n", turn);
#endif

	debug_assert(turn == m_ReadyTurn + 1);
	m_ReadyTurn = turn;
}

void CNetClientTurnManager::PostCommand(CScriptValRooted data)
{
#ifdef NETTURN_LOG
	NETTURN_LOG(L"PostCommand()\n");
#endif

	// Transmit command to server
	CSimulationMessage msg(m_Simulation2.GetScriptInterface(), m_ClientId, m_PlayerId, m_CurrentTurn + COMMAND_DELAY, data.get());
	CNetSession* session = m_NetClient.GetSession(0);
	m_NetClient.SendMessage(session, &msg);

	// Add to our local queue
	//AddCommand(m_ClientId, m_PlayerId, data, m_CurrentTurn + COMMAND_DELAY);
	// TODO: we should do this when the server stops sending our commands back to us
}

void CNetClientTurnManager::NotifyFinishedOwnCommands(u32 turn)
{
#ifdef NETTURN_LOG
	NETTURN_LOG(L"NotifyFinishedOwnCommands(%d)\n", turn);
#endif

	CEndCommandBatchMessage msg;
	msg.m_TurnLength = TURN_LENGTH;
	msg.m_Turn = turn;
	CNetSession* session = m_NetClient.GetSession(0);
	m_NetClient.SendMessage(session, &msg);
}

void CNetClientTurnManager::OnSimulationMessage(CSimulationMessage* msg)
{
	// Command received from the server - store it for later execution
	AddCommand(msg->m_Client, msg->m_Player, msg->m_Data, msg->m_Turn);
}



void CNetServerTurnManager::PostCommand(CScriptValRooted data)
{
#ifdef NETTURN_LOG
	NETTURN_LOG(L"PostCommand()\n");
#endif

	// Transmit command to all clients
	CSimulationMessage msg(m_Simulation2.GetScriptInterface(), m_ClientId, m_PlayerId, m_CurrentTurn + COMMAND_DELAY, data.get());
	m_NetServer.Broadcast(&msg);

	// Add to our local queue
	AddCommand(m_ClientId, m_PlayerId, data, m_CurrentTurn + COMMAND_DELAY);
}

void CNetServerTurnManager::NotifyFinishedOwnCommands(u32 turn)
{
#ifdef NETTURN_LOG
	NETTURN_LOG(L"NotifyFinishedOwnCommands(%d)\n", turn);
#endif

	NotifyFinishedClientCommands(m_ClientId, turn);
}

void CNetServerTurnManager::NotifyFinishedClientCommands(int client, u32 turn)
{
#ifdef NETTURN_LOG
	NETTURN_LOG(L"NotifyFinishedClientCommands(client=%d, turn=%d)\n", client, turn);
#endif

	// Must be a client we've already heard of
	debug_assert(m_ClientsReady.find(client) != m_ClientsReady.end());

	// Clients must advance one turn at a time
	debug_assert(turn == m_ClientsReady[client] + 1);
	m_ClientsReady[client] = turn;

	// See if all clients (including self) are ready for a new turn
	for (std::map<int, u32>::iterator it = m_ClientsReady.begin(); it != m_ClientsReady.end(); ++it)
	{
#ifdef NETTURN_LOG
		NETTURN_LOG(L"  %d: %d <=? %d\n", it->first, it->second, m_ReadyTurn);
#endif
		if (it->second <= m_ReadyTurn)
			return;
	}

	// Tell all clients that the next turn is ready
	CEndCommandBatchMessage msg;
	msg.m_TurnLength = TURN_LENGTH;
	msg.m_Turn = turn;
	m_NetServer.Broadcast(&msg);

	// Move ourselves to the next turn
	FinishedAllCommands(m_ReadyTurn + 1);
}

void CNetServerTurnManager::InitialiseClient(int client)
{
	debug_assert(m_ClientsReady.find(client) == m_ClientsReady.end());
	m_ClientsReady[client] = 1;

	// TODO: do we need some kind of UninitialiseClient in case they leave?
}

void CNetServerTurnManager::OnSimulationMessage(CSimulationMessage* msg)
{
	// Send it back to all clients immediately
	m_NetServer.Broadcast(msg);

	// TODO: we should do some validation of ownership (clients can't send commands on behalf of opposing players)

	// TODO: we shouldn't send the message back to the client that first sent it

	// Process it ourselves
	AddCommand(msg->m_Client, msg->m_Player, msg->m_Data, msg->m_Turn);
}

void CNetLocalTurnManager::PostCommand(CScriptValRooted data)
{
	// Add directly to the next turn, ignoring COMMAND_DELAY,
	// because we don't need to compensate for network latency
	AddCommand(m_ClientId, m_PlayerId, data, m_CurrentTurn + 1);
}

void CNetLocalTurnManager::NotifyFinishedOwnCommands(u32 turn)
{
	FinishedAllCommands(turn);
}

void CNetLocalTurnManager::OnSimulationMessage(CSimulationMessage* UNUSED(msg))
{
	debug_warn(L"This should never be called");
}
