#ifndef INCLUDED_NETWORK_SERVER
#define INCLUDED_NETWORK_SERVER

#include "Session.h"
#include "ps/GameAttributes.h"
#include "simulation/TurnManager.h"
#include "ps/scripting/JSMap.h"
#include "simulation/ScriptObject.h"

class CGame;

enum ENetServerState
{
	// We haven't opened the port yet, we're just setting some stuff up.
	// This is probably equivalent to the first "Start Network Game" screen
	NSS_PreBind,
	// The server is open and accepting connections. This is the screen where
	// rules are set up by the operator and where players join and select civs
	// and stuff.
	NSS_PreGame,
	// In-Game state: the one with all the killing ;-)
	NSS_InGame,
	// The game is over and someone has won. Players might linger to chat or
	// download the replay log.
	NSS_PostGame
};

class CNetServerSession;

class CNetServer:
	protected CServerSocket,
	protected CTurnManager,
	public CJSObject<CNetServer>
{
private:
	typedef std::map<int, CNetServerSession *> SessionMap;

	/*
		Every connected session is in m_Sessions as soon as the Handshake and
		Authentication stages are complete.
	*/
	SessionMap m_Sessions;
	CJSMap<SessionMap> m_JSI_Sessions;
	/*
		All sessions that have observer status (observer as in watcher - simple
		chatters don't have an entry here, only in m_Sessions).
		Sessions are added here after they have successfully requested observer
		status.
	*/
	std::vector <CNetServerSession *> m_Observers;
	
	ENetServerState m_ServerState;

	CGame *m_pGame;
	CGameAttributes *m_pGameAttributes;

	uint m_MaxObservers;
	int m_LastSessionID;

	CPlayer *m_pServerPlayer;

	CStrW m_Password;
	CStrW m_ServerPlayerName;
	CStrW m_ServerName;
	CStrW m_WelcomeMessage;
	
	int m_Port;
	
	CScriptObject m_OnChat;
	CScriptObject m_OnClientConnect;
	CScriptObject m_OnClientDisconnect;
	
	static CGameAttributes::UpdateCallback AttributeUpdate;
	static CPlayer::UpdateCallback PlayerAttributeUpdate;
	static PlayerSlotAssignmentCB PlayerSlotAssignmentCallback;
	
	void FillSetGameConfig(CSetGameConfig *pMsg);
	void FillSetPlayerConfig(CSetPlayerConfig *pMsg, CPlayer *pPlayer);
	static CNetMessage *CreatePlayerSlotAssignmentMessage(CPlayerSlot *slot);
	
	// JS Interface Methods
	bool JSI_Open(JSContext *cx, uintN argc, jsval *argv);
	
	// Synchronization object for batches
	CMutex m_Mutex;

protected:
	friend class CNetServerSession;

	// Assign a session ID to the session. Do this just before calling AddSession
	void AssignSessionID(CNetServerSession *pSession);
	// Add the session. This will be called after the session passes the
	// handshake and authentication stages. AssignSessionID should've been called
	// on the session prior to calling this method.
	void AddSession(CNetServerSession *pSession);

	// Remove the session from the server
	void RemoveSession(CNetServerSession *pSession);

	// Queue a command coming in from the wire. The command has been validated
	// by the caller.
	void QueueIncomingCommand(CNetMessage *pMsg);
	
	// Call the JS callback for incoming events
	void OnChat(const CStrW& from, const CStrW& message);
	void OnClientConnect(CNetServerSession *pSession);
	void OnClientDisconnect(CNetServerSession *pSession);
	
	// OVERRIDES FROM CServerSocket
	virtual void OnAccept(const CSocketAddress &);

	// OVERRIDES FROM CTurnManager
	virtual bool NewTurnReady();
	virtual void NewTurn();
	virtual void QueueLocalCommand(CNetMessage *pMsg);

// OVERRIDABLES
	// Will only be called from the Network Thread, by the OnAccept handler
	virtual CNetServerSession *CreateSession(CSocketInternal *pInt);

	// Ask the server if the session is allowed to start observing.
	//
	// Returns:
	//	true if the session should be made an observer
	//	false otherwise
	virtual bool AllowObserver(CNetServerSession *pSession);

public:
	CNetServer(CGame *pGame, CGameAttributes *pGameAttribs);
	virtual ~CNetServer();

	static void GetDefaultListenAddress(CSocketAddress &address);
	PS_RESULT Bind(const CSocketAddress &address);
	
	inline void SetPassword(const CStr& password)
	{	m_Password=password;	}

	inline const CStrW& GetServerPlayerName()
	{	return m_ServerPlayerName; }

	inline ENetServerState GetServerState()
	{	return m_ServerState; }

	int StartGame();

	void Broadcast(CNetMessage *);

	static void ScriptingInit();
};

extern CNetServer *g_NetServer;

#endif // INCLUDED_NETWORK_SERVER
