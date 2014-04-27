/* Copyright (C) 2011 Wildfire Games.
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

#ifndef NETCLIENT_H
#define NETCLIENT_H

#include "network/fsm.h"
#include "network/NetFileTransfer.h"
#include "network/NetHost.h"
#include "scriptinterface/ScriptVal.h"

#include "ps/CStr.h"

#include <deque>

class CGame;
class CNetClientSession;
class CNetClientTurnManager;
class CNetServer;
class ScriptInterface;

// NetClient session FSM states
enum
{
	NCS_UNCONNECTED,
	NCS_CONNECT,
	NCS_HANDSHAKE,
	NCS_AUTHENTICATE,
	NCS_INITIAL_GAMESETUP,
	NCS_PREGAME,
	NCS_LOADING,
	NCS_JOIN_SYNCING,
	NCS_INGAME
};

/**
 * Network client.
 * This code is run by every player (including the host, if they are not
 * a dedicated server).
 * It provides an interface between the GUI, the network (via CNetClientSession),
 * and the game (via CGame and CNetClientTurnManager).
 */
class CNetClient : public CFsm
{
	NONCOPYABLE(CNetClient);

	friend class CNetFileReceiveTask_ClientRejoin;

public:
	/**
	 * Construct a client associated with the given game object.
	 * The game must exist for the lifetime of this object.
	 */
	CNetClient(CGame* game);

	virtual ~CNetClient();

	/**
	 * Set the user's name that will be displayed to all players.
	 * This must not be called after the connection setup.
	 */
	void SetUserName(const CStrW& username);

	/**
	 * Set up a connection to the remote networked server.
	 * @param server IP address or host name to connect to
	 * @return true on success, false on connection failure
	 */
	bool SetupConnection(const CStr& server);

	/**
	 * Destroy the connection to the server.
	 * This client probably cannot be used again.
	 */
	void DestroyConnection();

	/**
	 * Poll the connection for messages from the server and process them, and send
	 * any queued messages.
	 * This must be called frequently (i.e. once per frame).
	 */
	void Poll();

	/**
	 * Flush any queued outgoing network messages.
	 * This should be called soon after sending a group of messages that may be batched together.
	 */
	void Flush();

	/**
	 * Retrieves the next queued GUI message, and removes it from the queue.
	 * The returned value is in the GetScriptInterface() JS context.
	 *
	 * This is the only mechanism for the networking code to send messages to
	 * the GUI - it is pull-based (instead of push) so the engine code does not
	 * need to know anything about the code structure of the GUI scripts.
	 *
	 * The structure of the messages is <code>{ "type": "...", ... }</code>.
	 * The exact types and associated data are not specified anywhere - the
	 * implementation and GUI scripts must make the same assumptions.
	 *
	 * @return next message, or the value 'undefined' if the queue is empty
	 */
	CScriptValRooted GuiPoll();

	/**
	 * Add a message to the queue, to be read by GuiPoll.
	 * The script value must be in the GetScriptInterface() JS context.
	 */
	void PushGuiMessage(const CScriptValRooted& message);

	/**
	 * Return a concatenation of all messages in the GUI queue,
	 * for test cases to easily verify the queue contents.
	 */
	std::wstring TestReadGuiMessages();

	/**
	 * Get the script interface associated with this network client,
	 * which is equivalent to the one used by the CGame in the constructor.
	 */
	ScriptInterface& GetScriptInterface();

	/**
	 * Send a message to the server.
	 * @param message message to send
	 * @return true on success
	 */
	bool SendMessage(const CNetMessage* message);

	/**
	 * Call when the network connection has been successfully initiated.
	 */
	void HandleConnect();

	/**
	 * Call when the network connection has been lost.
	 */
	void HandleDisconnect(u32 reason);

	/**
	 * Call when a message has been received from the network.
	 */
	bool HandleMessage(CNetMessage* message);

	/**
	 * Call when the game has started and all data files have been loaded,
	 * to signal to the server that we are ready to begin the game.
	 */
	void LoadFinished();

	void SendChatMessage(const std::wstring& text);
	
	void SendReadyMessage(const int status);

private:
	// Net message / FSM transition handlers
	static bool OnConnect(void* context, CFsmEvent* event);
	static bool OnHandshake(void* context, CFsmEvent* event);
	static bool OnHandshakeResponse(void* context, CFsmEvent* event);
	static bool OnAuthenticate(void* context, CFsmEvent* event);
	static bool OnChat(void* context, CFsmEvent* event);
	static bool OnReady(void* context, CFsmEvent* event);
	static bool OnGameSetup(void* context, CFsmEvent* event);
	static bool OnPlayerAssignment(void* context, CFsmEvent* event);
	static bool OnInGame(void* context, CFsmEvent* event);
	static bool OnGameStart(void* context, CFsmEvent* event);
	static bool OnJoinSyncStart(void* context, CFsmEvent* event);
	static bool OnJoinSyncEndCommandBatch(void* context, CFsmEvent* event);
	static bool OnLoadedGame(void* context, CFsmEvent* event);

	/**
	 * Take ownership of a session object, and use it for all network communication.
	 */
	void SetAndOwnSession(CNetClientSession* session);

	/**
	 * Push a message onto the GUI queue listing the current player assignments.
	 */
	void PostPlayerAssignmentsToScript();

	CGame *m_Game;
	CStrW m_UserName;

	/// Current network session (or NULL if not connected)
	CNetClientSession* m_Session;

	/// Turn manager associated with the current game (or NULL if we haven't started the game yet)
	CNetClientTurnManager* m_ClientTurnManager;

	/// Unique-per-game identifier of this client, used to identify the sender of simulation commands
	u32 m_HostID;

	/// Latest copy of game setup attributes heard from the server
	CScriptValRooted m_GameAttributes;

	/// Latest copy of player assignments heard from the server
	PlayerAssignmentMap m_PlayerAssignments;

	/// Globally unique identifier to distinguish users beyond the lifetime of a single network session
	CStr m_GUID;

	/// Queue of messages for GuiPoll
	std::deque<CScriptValRooted> m_GuiMessageQueue;

	/// Serialized game state received when joining an in-progress game
	std::string m_JoinSyncBuffer;
};

/// Global network client for the standard game
extern CNetClient *g_NetClient;

#endif	// NETCLIENT_H
