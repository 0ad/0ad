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

#include "NetClient.h"

#include "NetMessage.h"
#include "NetSession.h"
#include "NetTurnManager.h"

#include "lib/sysdep/sysdep.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

CNetClient *g_NetClient = NULL;

CNetClient::CNetClient(CGame* game) :
	m_Session(NULL),
	m_UserName(L"anonymous"),
	m_GUID(GenerateGUID()), m_HostID((u32)-1), m_ClientTurnManager(NULL), m_Game(game)
{
	m_Game->SetTurnManager(NULL); // delete the old local turn manager so we don't accidentally use it

	void* context = this;

	// Set up transitions for session
	AddTransition(NCS_UNCONNECTED, (uint)NMT_CONNECT_COMPLETE, NCS_CONNECT, (void*)&OnConnect, context);

	AddTransition(NCS_CONNECT, (uint)NMT_SERVER_HANDSHAKE, NCS_HANDSHAKE, (void*)&OnHandshake, context);

	AddTransition(NCS_HANDSHAKE, (uint)NMT_SERVER_HANDSHAKE_RESPONSE, NCS_AUTHENTICATE, (void*)&OnHandshakeResponse, context);

	AddTransition(NCS_AUTHENTICATE, (uint)NMT_AUTHENTICATE_RESULT, NCS_INITIAL_GAMESETUP, (void*)&OnAuthenticate, context);

	AddTransition(NCS_INITIAL_GAMESETUP, (uint)NMT_GAME_SETUP, NCS_PREGAME, (void*)&OnGameSetup, context);

	AddTransition(NCS_PREGAME, (uint)NMT_GAME_SETUP, NCS_PREGAME, (void*)&OnGameSetup, context);
	AddTransition(NCS_PREGAME, (uint)NMT_PLAYER_ASSIGNMENT, NCS_PREGAME, (void*)&OnPlayerAssignment, context);
	AddTransition(NCS_PREGAME, (uint)NMT_GAME_START, NCS_LOADING, (void*)&OnGameStart, context);

	AddTransition(NCS_LOADING, (uint)NMT_GAME_SETUP, NCS_LOADING, (void*)&OnGameSetup, context);
	AddTransition(NCS_LOADING, (uint)NMT_PLAYER_ASSIGNMENT, NCS_LOADING, (void*)&OnPlayerAssignment, context);
	AddTransition(NCS_LOADING, (uint)NMT_LOADED_GAME, NCS_INGAME, (void*)&OnLoadedGame, context);

	AddTransition(NCS_INGAME, (uint)NMT_GAME_SETUP, NCS_INGAME, (void*)&OnGameSetup, context);
	AddTransition(NCS_INGAME, (uint)NMT_PLAYER_ASSIGNMENT, NCS_INGAME, (void*)&OnPlayerAssignment, context);
	AddTransition(NCS_INGAME, (uint)NMT_SIMULATION_COMMAND, NCS_INGAME, (void*)&OnInGame, context);
	AddTransition(NCS_INGAME, (uint)NMT_SYNC_ERROR, NCS_INGAME, (void*)&OnInGame, context);
	AddTransition(NCS_INGAME, (uint)NMT_END_COMMAND_BATCH, NCS_INGAME, (void*)&OnInGame, context);

	// TODO: add chat

	// Set first state
	SetFirstState(NCS_UNCONNECTED);
}

CNetClient::~CNetClient()
{
	delete m_Session;
}

void CNetClient::SetUserName(const CStrW& username)
{
	debug_assert(!m_Session); // must be called before we start the connection

	m_UserName = username;
}

bool CNetClient::SetupConnection(const CStr& server)
{
	CNetClientSessionRemote* session = new CNetClientSessionRemote(*this);
	bool ok = session->Connect(PS_DEFAULT_PORT, server);
	SetAndOwnSession(session);
	return ok;
}

void CNetClient::SetupLocalConnection(CNetServer& server)
{
	CNetClientSessionLocal* session = new CNetClientSessionLocal(*this, server);
	SetAndOwnSession(session);
}

void CNetClient::SetAndOwnSession(CNetClientSession* session)
{
	delete m_Session;
	m_Session = session;
}

void CNetClient::Poll()
{
	if (m_Session)
		m_Session->Poll();
}

CScriptValRooted CNetClient::GuiPoll()
{
	if (m_GuiMessageQueue.empty())
		return CScriptValRooted();

	CScriptValRooted r = m_GuiMessageQueue.front();
	m_GuiMessageQueue.pop_front();
	return r;
}

void CNetClient::PushGuiMessage(const CScriptValRooted& message)
{
	debug_assert(!message.undefined());

	m_GuiMessageQueue.push_back(message);
}

std::wstring CNetClient::TestReadGuiMessages()
{
	std::wstring r;
	while (true)
	{
		CScriptValRooted msg = GuiPoll();
		if (msg.undefined())
			break;
		r += GetScriptInterface().ToString(msg.get()) + L"\n";
	}
	return r;
}

ScriptInterface& CNetClient::GetScriptInterface()
{
	return m_Game->GetSimulation2()->GetScriptInterface();
}

void CNetClient::PostPlayerAssignmentsToScript()
{
	CScriptValRooted msg;
	GetScriptInterface().Eval("({'type':'players', 'hosts':{}})", msg);

	CScriptValRooted hosts;
	GetScriptInterface().GetProperty(msg.get(), "hosts", hosts);

	for (PlayerAssignmentMap::iterator it = m_PlayerAssignments.begin(); it != m_PlayerAssignments.end(); ++it)
	{
		CScriptValRooted host;
		GetScriptInterface().Eval("({})", host);
		GetScriptInterface().SetProperty(host.get(), "name", std::wstring(it->second.m_Name), false);
		GetScriptInterface().SetProperty(host.get(), "player", it->second.m_PlayerID, false);
		GetScriptInterface().SetProperty(hosts.get(), it->first, host, false);
	}

	PushGuiMessage(msg);
}

bool CNetClient::SendMessage(const CNetMessage* message)
{
	return m_Session->SendMessage(message);
}

void CNetClient::HandleConnect()
{
	Update((uint)NMT_CONNECT_COMPLETE, NULL);
}

void CNetClient::HandleDisconnect()
{
	// TODO: should do something
}

bool CNetClient::HandleMessage(CNetMessage* message)
{
	// Update FSM
	bool ok = Update(message->GetType(), message);
	if (!ok)
		LOGERROR(L"Net client: Error running FSM update (type=%d state=%d)", (int)message->GetType(), (int)GetCurrState());
	return ok;
}

void CNetClient::LoadFinished()
{
	m_Game->ChangeNetStatus(CGame::NET_WAITING_FOR_CONNECT);

	CLoadedGameMessage loaded;
	SendMessage(&loaded);
}

bool CNetClient::OnConnect(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_CONNECT_COMPLETE);

	CNetClient* client = (CNetClient*)context;

	CScriptValRooted msg;
	client->GetScriptInterface().Eval("({'type':'netstatus','status':'connected'})", msg);
	client->PushGuiMessage(msg);

	return true;
}

bool CNetClient::OnHandshake(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_SERVER_HANDSHAKE);

	CNetClient* client = (CNetClient*)context;

	CCliHandshakeMessage handshake;
	handshake.m_MagicResponse = PS_PROTOCOL_MAGIC_RESPONSE;
	handshake.m_ProtocolVersion = PS_PROTOCOL_VERSION;
	handshake.m_SoftwareVersion = PS_PROTOCOL_VERSION;
	client->SendMessage(&handshake);

	return true;
}

bool CNetClient::OnHandshakeResponse(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_SERVER_HANDSHAKE_RESPONSE);

	CNetClient* client = (CNetClient*)context;

	CAuthenticateMessage authenticate;
	authenticate.m_GUID = client->m_GUID;
	authenticate.m_Name = client->m_UserName;
	authenticate.m_Password = L""; // TODO
	client->SendMessage(&authenticate);

	return true;
}

bool CNetClient::OnAuthenticate(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_AUTHENTICATE_RESULT);

	CNetClient* client = (CNetClient*)context;

	CAuthenticateResultMessage* message = (CAuthenticateResultMessage*)event->GetParamRef();

	LOGMESSAGE(L"Net: Authentication result: host=%d, %ls", message->m_HostID, message->m_Message.c_str() );

	client->m_HostID = message->m_HostID;

	CScriptValRooted msg;
	client->GetScriptInterface().Eval("({'type':'netstatus','status':'authenticated'})", msg);
	client->PushGuiMessage(msg);

	return true;
}

bool CNetClient::OnGameSetup(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_GAME_SETUP);

	CNetClient* client = (CNetClient*)context;

	CGameSetupMessage* message = (CGameSetupMessage*)event->GetParamRef();

	client->m_GameAttributes = message->m_Data;

	CScriptValRooted msg;
	client->GetScriptInterface().Eval("({'type':'gamesetup'})", msg);
	client->GetScriptInterface().SetProperty(msg.get(), "data", message->m_Data, false);
	client->PushGuiMessage(msg);

	return true;
}

bool CNetClient::OnPlayerAssignment(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_PLAYER_ASSIGNMENT);

	CNetClient* client = (CNetClient*)context;

	CPlayerAssignmentMessage* message = (CPlayerAssignmentMessage*)event->GetParamRef();

	// Unpack the message
	client->m_PlayerAssignments.clear();
	for (size_t i = 0; i < message->m_Hosts.size(); ++i)
	{
		PlayerAssignment assignment;
		assignment.m_Name = message->m_Hosts[i].m_Name;
		assignment.m_PlayerID = message->m_Hosts[i].m_PlayerID;
		client->m_PlayerAssignments[message->m_Hosts[i].m_GUID] = assignment;
	}

	client->PostPlayerAssignmentsToScript();

	return true;
}

bool CNetClient::OnGameStart(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_GAME_START);

	CNetClient* client = (CNetClient*)context;

	// Find the player assigned to our GUID
	int player = -1;
	if (client->m_PlayerAssignments.find(client->m_GUID) != client->m_PlayerAssignments.end())
		player = client->m_PlayerAssignments[client->m_GUID].m_PlayerID;

	client->m_ClientTurnManager = new CNetClientTurnManager(*client->m_Game->GetSimulation2(), *client, client->m_HostID);

	client->m_Game->SetPlayerID(player);
	client->m_Game->StartGame(client->m_GameAttributes);

	CScriptValRooted msg;
	client->GetScriptInterface().Eval("({'type':'start'})", msg);
	client->PushGuiMessage(msg);

	return true;
}

bool CNetClient::OnLoadedGame(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_LOADED_GAME);

	CNetClient* client = (CNetClient*)context;

	// All players have loaded the game - start running the turn manager
	// so that the game begins
	client->m_Game->SetTurnManager(client->m_ClientTurnManager);
	client->m_Game->ChangeNetStatus(CGame::NET_NORMAL);

	return true;
}

bool CNetClient::OnInGame(void *context, CFsmEvent* event)
{
	// TODO: should split each of these cases into a separate method

	CNetClient* client = (CNetClient*)context;

	CNetMessage* message = (CNetMessage*)event->GetParamRef();
	if (message)
	{
		if (message->GetType() == NMT_SIMULATION_COMMAND)
		{
			CSimulationMessage* simMessage = static_cast<CSimulationMessage*> (message);
			client->m_ClientTurnManager->OnSimulationMessage(simMessage);
		}
		else if (message->GetType() == NMT_SYNC_ERROR)
		{
			CSyncErrorMessage* syncMessage = static_cast<CSyncErrorMessage*> (message);
			client->m_ClientTurnManager->OnSyncError(syncMessage->m_Turn, syncMessage->m_HashExpected);
		}
		else if (message->GetType() == NMT_END_COMMAND_BATCH)
		{
			CEndCommandBatchMessage* endMessage = static_cast<CEndCommandBatchMessage*> (message);
			client->m_ClientTurnManager->FinishedAllCommands(endMessage->m_Turn);
		}
	}

	return true;
}

CStr CNetClient::GenerateGUID()
{
	// TODO: Ideally this will be guaranteed unique (and verified
	// cryptographically) since we'll rely on it to identify hosts
	// and associate them with player controls (e.g. to support
	// leaving/rejoining in-progress games), and we don't want
	// a host to masquerade as someone else.
	// For now, just try to pick a very random number.

	CStr guid;
	for (size_t i = 0; i < 2; ++i)
	{
		u32 r = 0;
		sys_generate_random_bytes((u8*)&r, sizeof(r));
		char buf[32];
		sprintf_s(buf, ARRAY_SIZE(buf), "%08X", r);
		guid += buf;
	}

	return guid;
}
