/* Copyright (C) 2022 Wildfire Games.
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

#include "NetFileTransfer.h"
#include "NetHost.h"
#include "lib/config2.h"
#include "lib/types.h"
#include "scriptinterface/ScriptTypes.h"

#include <ctime>
#include <mutex>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>
#include <thread>

class CNetServerSession;
class CNetServerTurnManager;
class CFsmEvent;
class CPlayerAssignmentMessage;
class CNetStatsTable;
class CSimulationMessage;
class ScriptInterface;
class ScriptRequest;

class CNetServerWorker;

enum NetServerState
{
	// We haven't opened the port yet, we're just setting some stuff up.
	// The worker thread has not been started.
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

	// The client has handshook and we're waiting for its lobby authentication message
	NSS_LOBBY_AUTHENTICATE,

	// The client has handshook and we're waiting for its authentication message,
	// to find its name and check its password etc
	NSS_AUTHENTICATE,

	// The client has fully joined, and is in the pregame setup stage
	// or is loading the game.
	// Server must be in SERVER_STATE_PREGAME or SERVER_STATE_LOADING.
	NSS_PREGAME,

	// The client has authenticated but the game was already started,
	// so it's synchronising with the game state from other clients
	NSS_JOIN_SYNCING,

	// The client is running the game.
	// Server must be in SERVER_STATE_LOADING or SERVER_STATE_INGAME.
	NSS_INGAME
};

/**
 * Network server interface. Handles all the coordination between players.
 * One person runs this object, and every player (including the host) connects their CNetClient to it.
 *
 * The actual work is performed by CNetServerWorker in a separate thread.
 */
class CNetServer
{
	NONCOPYABLE(CNetServer);
public:
	/**
	 * Construct a new network server.
	 * once this many players are connected (intended for the command-line testing mode).
	 */
	CNetServer(bool useLobbyAuth = false);

	~CNetServer();

	/**
	 * Begin listening for network connections.
	 * This function is synchronous (it won't return until the connection is established).
	 * @return true on success, false on error (e.g. port already in use)
	 */
	bool SetupConnection(const u16 port);

	/**
	 * Call from the GUI to asynchronously notify all clients that they should start loading the game.
	 * UpdateInitAttributes must be called at least once.
	 */
	void StartGame();

	/**
	 * Call from the GUI to update the game setup attributes.
	 * The changes won't be propagated to clients until game start.
	 * @param attrs init attributes, in the script context of rq
	 */
	void UpdateInitAttributes(JS::MutableHandleValue attrs, const ScriptRequest& rq);

	/**
	 * Set the turn length to a fixed value.
	 * TODO: we should replace this with some adapative lag-dependent computation.
	 */
	void SetTurnLength(u32 msecs);

	bool UseLobbyAuth() const;

	void OnLobbyAuth(const CStr& name, const CStr& token);

	void SendHolePunchingMessage(const CStr& ip, u16 port);

	void SetConnectionData(const CStr& ip, u16 port);
	bool SetConnectionDataViaSTUN();

	bool GetUseSTUN() const;

	/**
	 * Return the externally accessible IP.
	 */
	CStr GetPublicIp() const;

	/**
	 * Return the externally accessible port.
	 */
	u16 GetPublicPort() const;

	/**
	 * Return the serving port on the local machine.
	 */
	u16 GetLocalPort() const;

	/**
	 * Check if password is valid. If is not, increase number of failed attempts of the lobby user.
	 * This is used without established direct session with the client, to prevent brute force attacks
	 * when guessing password trying to get connection data from the host.
	 * @return true iff password is valid
	 */
	bool CheckPasswordAndIncrement(const std::string& username, const std::string& password, const std::string& salt);

	/**
	 * Check if user reached certain number of failed attempts.
	 * @see m_BanAfterNumberOfTries
	 * @see CheckPasswordAndBan
	 */
	bool IsBanned(const std::string& username) const;

	void SetPassword(const CStr& password);

	void SetControllerSecret(const std::string& secret);

private:
	CNetServerWorker* m_Worker;
	const bool m_LobbyAuth;
	bool m_UseSTUN;
	u16 m_PublicPort;
	CStr m_PublicIp;
	CStr m_Password;
	std::unordered_map<std::string, int> m_FailedAttempts;
};

/**
 * Network server worker thread.
 * (This is run in a thread so that client/server communication is not delayed
 * by the host player's framerate - the only delay should be the network latency.)
 *
 * Thread-safety:
 * - SetupConnection and constructor/destructor must be called from the main thread.
 * - The main thread may push commands onto the Queue members,
 *   while holding the m_WorkerMutex lock.
 * - Public functions (SendMessage, Broadcast) must be called from the network
 *   server thread.
 */
class CNetServerWorker
{
	NONCOPYABLE(CNetServerWorker);

public:
	// Public functions for CNetSession/CNetServerTurnManager to use:

	/**
	 * Send a message to the given network peer.
	 */
	bool SendMessage(ENetPeer* peer, const CNetMessage* message);

	/**
	 * Disconnects a player from gamesetup or session.
	 */
	void KickPlayer(const CStrW& playerName, const bool ban);

	/**
	 * Send a message to all clients who match one of the given states.
	 */
	bool Broadcast(const CNetMessage* message, const std::vector<NetServerSessionState>& targetStates);

private:
	friend class CNetServer;
	friend class CNetFileReceiveTask_ServerRejoin;

	CNetServerWorker(bool useLobbyAuth);
	~CNetServerWorker();

	bool CheckPassword(const std::string& password, const std::string& salt) const;

	void SetPassword(const CStr& hashedPassword);

	void SetControllerSecret(const std::string& secret);

	/**
	 * Begin listening for network connections.
	 * @return true on success, false on error (e.g. port already in use)
	 */
	bool SetupConnection(const u16 port);

	/**
	 * The given GUID will be (re)assigned to the given player ID.
	 * Any player currently using that ID will be unassigned.
	 */
	void AssignPlayer(int playerID, const CStr& guid);

	/**
	 * Switch in game mode and notify all clients to start the game.
	 */
	void StartGame(const CStr& initAttribs);

	/**
	 * Make a player name 'nicer' by limiting the length and removing forbidden characters etc.
	 */
	static CStrW SanitisePlayerName(const CStrW& original);

	/**
	 * Make a player name unique, if it matches any existing session's name.
	 */
	CStrW DeduplicatePlayerName(const CStrW& original);

	/**
	 * Get the script context used for init attributes.
	 */
	const ScriptInterface& GetScriptInterface();

	/**
	 * Set the turn length to a fixed value.
	 * TODO: we should replace this with some adaptive lag-dependent computation.
	 */
	void SetTurnLength(u32 msecs);

	void ProcessLobbyAuth(const CStr& name, const CStr& token);

	void AddPlayer(const CStr& guid, const CStrW& name);
	void RemovePlayer(const CStr& guid);
	void SendPlayerAssignments();
	void ClearAllPlayerReady();

	void SetupSession(CNetServerSession* session);
	bool HandleConnect(CNetServerSession* session);

	void OnUserJoin(CNetServerSession* session);
	void OnUserLeave(CNetServerSession* session);

	static bool OnClientHandshake(void* context, CFsmEvent* event);
	static bool OnAuthenticate(void* context, CFsmEvent* event);
	static bool OnSimulationCommand(void* context, CFsmEvent* event);
	static bool OnSyncCheck(void* context, CFsmEvent* event);
	static bool OnEndCommandBatch(void* context, CFsmEvent* event);
	static bool OnChat(void* context, CFsmEvent* event);
	static bool OnReady(void* context, CFsmEvent* event);
	static bool OnClearAllReady(void* context, CFsmEvent* event);
	static bool OnGameSetup(void* context, CFsmEvent* event);
	static bool OnAssignPlayer(void* context, CFsmEvent* event);
	static bool OnGameStart(void* context, CFsmEvent* event);
	static bool OnLoadedGame(void* context, CFsmEvent* event);
	static bool OnJoinSyncingLoadedGame(void* context, CFsmEvent* event);
	static bool OnRejoined(void* context, CFsmEvent* event);
	static bool OnKickPlayer(void* context, CFsmEvent* event);
	static bool OnDisconnect(void* context, CFsmEvent* event);
	static bool OnClientPaused(void* context, CFsmEvent* event);

	/**
	 * Checks if all clients have finished loading.
	 * If so informs the clients about that and change the server state.
	 *
	 * Returns if all clients finished loading.
	 */
	bool CheckGameLoadStatus(CNetServerSession* changedSession);

	void ConstructPlayerAssignmentMessage(CPlayerAssignmentMessage& message);

	void HandleMessageReceive(const CNetMessage* message, CNetServerSession* session);

	/**
	 * Send a network warning if the connection to a client is being lost or has bad latency.
	 */
	void CheckClientConnections();

	void SendHolePunchingMessage(const CStr& ip, u16 port);

	/**
	 * Internal script context for (de)serializing script messages,
	 * and for storing init attributes.
	 * (TODO: we shouldn't bother deserializing (except for debug printing of messages),
	 * we should just forward messages blindly and efficiently.)
	 */
	ScriptInterface* m_ScriptInterface;

	PlayerAssignmentMap m_PlayerAssignments;

	/**
	 * Stores the most current init attributes.
	 * NB: this is not guaranteed to be up-to-date until the server is LOADING or INGAME.
	 * At that point, the settings are frozen and ought to be identical to the simulation Init Attributes.
	 */
	JS::PersistentRootedValue m_InitAttributes;

	/**
	 * Whether this match requires lobby authentication.
	 */
	const bool m_LobbyAuth;

	ENetHost* m_Host;
	std::vector<CNetServerSession*> m_Sessions;

	CNetStatsTable* m_Stats;

	NetServerState m_State;

	CStrW m_ServerName;

	std::vector<u32> m_BannedIPs;
	std::vector<CStrW> m_BannedPlayers;

	CStr m_Password;

	/**
	 * Holds the GUIDs of all currently paused players.
	 */
	std::vector<CStr> m_PausingPlayers;

	u32 m_NextHostID;

	CNetServerTurnManager* m_ServerTurnManager;

	/**
	 * The GUID of the client in control of the game (the 'host' from the players' perspective).
	 */
	CStr m_ControllerGUID;

	/**
	 * The 'secret' used to identify the controller of the game.
	 */
	std::string m_ControllerSecret;

	/**
	 * A copy of all simulation commands received so far, indexed by
	 * turn number, to simplify support for rejoining etc.
	 * TODO: verify this doesn't use too much RAM.
	 */
	std::vector<std::vector<CSimulationMessage>> m_SavedCommands;

	/**
	 * The latest copy of the simulation state, received from an existing
	 * client when a new client has asked to rejoin the game.
	 */
	std::string m_JoinSyncFile;

	/**
	 *  Time when the clients connections were last checked for timeouts and latency.
	 */
	std::time_t m_LastConnectionCheck;

private:
	// Thread-related stuff:

#if CONFIG2_MINIUPNPC
	/**
	 * Try to find a UPnP root on the network and setup port forwarding.
	 */
	static void SetupUPnP();
	std::thread m_UPnPThread;
#endif

	static void RunThread(CNetServerWorker* data);
	void Run();
	bool RunStep();

	std::thread m_WorkerThread;
	std::mutex m_WorkerMutex;

	// protected by m_WorkerMutex
	bool m_Shutdown;

	// Queues for messages sent by the game thread (protected by m_WorkerMutex):
	std::vector<bool> m_StartGameQueue;
	std::vector<std::string> m_InitAttributesQueue;
	std::vector<std::pair<CStr, CStr>> m_LobbyAuthQueue;
	std::vector<u32> m_TurnLengthQueue;
};

/// Global network server for the standard game
extern CNetServer *g_NetServer;

#endif // NETSERVER_H
