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

#ifndef NETSERVER_H
#define NETSERVER_H

#include "NetHost.h"

#include "scriptinterface/ScriptVal.h"

#include <vector>

class CNetServerSession;
class CNetServerTurnManager;
class CFsmEvent;
class ScriptInterface;
class CPlayerAssignmentMessage;
class CNetStatsTable;

enum NetServerState
{
	// We haven't opened the port yet, we're just setting some stuff up.
	// This is probably equivalent to the first "Start Network Game" screen
	SERVER_STATE_UNCONNECTED,

	// The server is open and accepting connections. This is the screen where
	// rules are set up by the operator and where players join and select civs
	// and stuff.
	SERVER_STATE_PREGAME,

	// All the hosts are connected and are loading the game
	SERVER_STATE_LOADING,

	// The one with all the killing ;-)
	SERVER_STATE_INGAME,

	// The game is over and someone has won. Players might linger to chat or
	// download the replay log.
	SERVER_STATE_POSTGAME
};

/**
 * Server session representation of client state
 */
enum NetServerSessionState
{
	// The client has disconnected or been disconnected
	NSS_UNCONNECTED,

	// The client has just connected and we're waiting for its handshake message,
	// to agree on the protocol version
	NSS_HANDSHAKE,

	// The client has handshook and we're waiting for its authentication message,
	// to find its name and check its password etc
	NSS_AUTHENTICATE,

	// The client has fully joined, and is in the pregame setup stage
	// or is loading the game.
	// Server must be in SERVER_STATE_PREGAME or SERVER_STATE_LOADING.
	NSS_PREGAME,

	// The client is running the game.
	// Server must be in SERVER_STATE_LOADING or SERVER_STATE_INGAME.
	NSS_INGAME
};

/**
 * Network server.
 * Handles all the coordination between players.
 * One person runs this object, and every player (including the host) connects their CNetClient to it.
 *
 * TODO: ideally the ENet server would run in a separate thread so it can receive
 * and forward messages with minimal latency. But that's not supported now.
 */
class CNetServer
{
	NONCOPYABLE(CNetServer);
public:
	CNetServer();
	virtual ~CNetServer();

	/**
	 * Get the current server's connection/game state.
	 */
	NetServerState GetState() const { return m_State; }

	/**
	 * Begin listening for network connections.
	 * @return true on success, false on error (e.g. port already in use)
	 */
	bool SetupConnection();

	/**
	 * Poll the connections for messages from clients and process them, and send
	 * any queued messages.
	 * This must be called frequently (i.e. once per frame).
	 */
	virtual void Poll();

	/**
	 * Flush any queued outgoing network messages.
	 * This should be called soon after sending a group of messages that may be batched together.
	 */
	void Flush();

	/**
	 * Send a message to the given network peer.
	 */
	bool SendMessage(ENetPeer* peer, const CNetMessage* message);

	/**
	 * Send a message to all clients who have completed the full connection process
	 * (i.e. are in the pre-game or in-game states).
	 */
	bool Broadcast(const CNetMessage* message);

	/**
	 * Call from the GUI to update the player assignments.
	 * The given GUID will be (re)assigned to the given player ID.
	 * Any player currently using that ID will be unassigned.
	 * The changes will be propagated to all clients.
	 */
	void AssignPlayer(int playerID, const CStr& guid);

	/**
	 * Call from the GUI to notify all clients that they should start loading the game.
	 */
	void StartGame();

	/**
	 * Call from the GUI to update the game setup attributes.
	 * This must be called at least once before starting the game.
	 * The changes will be propagated to all clients.
	 * @param attrs game attributes, in the script context of GetScriptInterface()
	 */
	void UpdateGameAttributes(const CScriptValRooted& attrs);

	/**
	 * Make a player name 'nicer' by limiting the length and removing forbidden characters etc.
	 */
	static CStrW SanitisePlayerName(const CStrW& original);

	/**
	 * Make a player name unique, if it matches any existing session's name.
	 */
	CStrW DeduplicatePlayerName(const CStrW& original);

	/**
	 * Get the script context used for game attributes.
	 */
	ScriptInterface& GetScriptInterface();

protected:
	/// Callback for autostart; called when a player has finished connecting
	virtual void OnAddPlayer() { }
	/// Callback for autostart; called when a player has left the game
	virtual void OnRemovePlayer() { }

private:
	void AddPlayer(const CStr& guid, const CStrW& name);
	void RemovePlayer(const CStr& guid);
	void SendPlayerAssignments();
	PlayerAssignmentMap m_PlayerAssignments;

	CScriptValRooted m_GameAttributes;

	void SetupSession(CNetServerSession* session);
	bool HandleConnect(CNetServerSession* session);

	void OnUserJoin(CNetServerSession* session);
	void OnUserLeave(CNetServerSession* session);

	static bool OnClientHandshake(void* context, CFsmEvent* event);
	static bool OnAuthenticate(void* context, CFsmEvent* event);
	static bool OnInGame(void* context, CFsmEvent* event);
	static bool OnChat(void* context, CFsmEvent* event);
	static bool OnLoadedGame(void* context, CFsmEvent* event);
	static bool OnDisconnect(void* context, CFsmEvent* event);

	void CheckGameLoadStatus(CNetServerSession* changedSession);

	void ConstructPlayerAssignmentMessage(CPlayerAssignmentMessage& message);

	void HandleMessageReceive(const CNetMessage* message, CNetServerSession* session);

	/**
	 * Internal script context for (de)serializing script messages.
	 * (TODO: we shouldn't bother deserializing (except for debug printing of messages),
	 * we should just forward messages blindly and efficiently.)
	 */
	ScriptInterface* m_ScriptInterface;

	ENetHost* m_Host;
	std::vector<CNetServerSession*> m_Sessions;

	CNetStatsTable* m_Stats;

	NetServerState m_State;

	CStrW m_ServerName;
	CStrW m_WelcomeMessage;

	u32 m_NextHostID;

	CNetServerTurnManager* m_ServerTurnManager;
};

/// Global network server for the standard game
extern CNetServer *g_NetServer;

#endif // NETSERVER_H
