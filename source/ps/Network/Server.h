#ifndef _Network_NetServer_H
#define _Network_NetServer_H

#include "Network/Session.h"
#include "Network/ServerSession.h"
#include "Game.h"
#include "TurnManager.h"

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

class CNetServer:
	protected CServerSocket,
	protected CTurnManager,
	public CJSObject<CNetServer>
{
private:
	/*
		Every connected session is in m_Sessions as soon as the Handshake and
		Authentication stages are complete.
	*/
	std::vector <CNetServerSession *> m_Sessions;
	/*
		All sessions currently associated with a player. Subset of m_Sessions.
	*/
	std::vector <CNetServerSession *> m_PlayerSessions;
	/*
		All sessions that have observer status (observer as in watcher - simple
		chatters don't have an entry here, only in m_Sessions).
		Sessions are added here after they have successfully requested observer
		status.
	*/
	std::vector <CNetServerSession *> m_Observers;
	
	uint m_NumPlayers;
	uint m_MaxObservers;

	ENetServerState m_ServerState;

	CGame *m_pGame;
	CGameAttributes *m_pGameAttributes;

	CPlayer *m_pServerPlayer;

	CStrW m_Password;
	CStrW m_ServerPlayerName;
	CStrW m_ServerName;
	CStrW m_WelcomeMessage;
	
	int m_Port;
	
	static CGameAttributes::UpdateCallback AttributeUpdate;
	static CPlayer::UpdateCallback PlayerAttributeUpdate;
	
	void FillSetGameConfig(CSetGameConfig *pMsg);

protected:
	friend class CNetServerSession;

	// Try to add the session as a newly connected player. If that is not
	// possible, make the session an observer.
	//
	// Returns:
	//	true. The session has been allocated a player slot
	//	false: All player slots busy, the session should be made an observer
	bool AddNewPlayer(CNetServerSession *pSession);

	// Remove the session from all the relevant lists.
	// NOTE: Currently unsafe to call for observers or players
	void RemoveSession(CNetServerSession *pSession);

	// Queue a command coming in from the wire. The command has been validated
	// by the caller.
	void QueueIncomingCommand(CNetMessage *pMsg);
	
	// OVERRIDES FROM CServerSocket
	virtual void OnAccept(const CSocketAddress &);

	// OVERRIDES FROM CTurnManager
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

	static void GetDefaultListenAddress(CSocketAddress &address);
	PS_RESULT Bind(const CSocketAddress &address);
	
	inline void SetPassword(CStr password)
	{	m_Password=password;	}

	inline CStrW GetServerPlayerName()
	{	return m_ServerPlayerName; }

	inline ENetServerState GetServerState()
	{	return m_ServerState; }

	int StartGame();

	void Broadcast(CNetMessage *);

	// JS Interface Methods
	bool JSI_Open(JSContext *cx, uintN argc, jsval *argv);
};

extern CNetServer *g_NetServer;

#endif // _Network_NetServer_H
