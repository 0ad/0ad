#ifndef _Network_NetServer_H
#define _Network_NetServer_H

#include "Network/Session.h"
#include "Game.h"
#include "TurnManager.h"

class CNetServer;
class CPlayer;

class CNetServerSession: public CNetSession
{
	CNetServer *m_pServer;
	CPlayer *m_pPlayer;
	bool m_IsObserver;

public:
	virtual ~CNetServerSession();

	inline CNetServerSession(CNetServer *pServer, CSocketInternal *pInt, MessageHandler *pMsgHandler=HandshakeHandler):
		CNetSession(pInt, pMsgHandler),
		m_pServer(pServer)
	{}

	inline bool IsObserver()
	{	return m_IsObserver; }

	// Called by server when starting the game, after sending NMT_StartGame to
	// all connected clients.
	void StartGame();

	static MessageHandler HandshakeHandler;
	static MessageHandler AuthenticateHandler;
	static MessageHandler PreGameHandler;
	static MessageHandler ObserverHandler;
	static MessageHandler ChatHandler;
	static MessageHandler InGameHandler;
};

enum ENetServerState
{
	NSS_PreBind,
	NSS_PreGame,
	NSS_InGame,
	NSS_PostGame
};

class CNetServerAttributes: public CAttributeMap
{
public:
	CNetServerAttributes();
};

class CNetServer: protected CServerSocket, protected CTurnManager
{
private:
	std::vector <CNetServerSession *> m_Sessions;
	
	std::vector <CNetServerSession *> m_PlayerSessions;
	uint m_NumPlayers;

	CMutex m_Mutex;
	ENetServerState m_ServerState;

	std::vector <CNetServerSession *> m_Observers;
	uint m_MaxObservers;

	CGame *m_pGame;
	CGameAttributes *m_pGameAttributes;
	CNetServerAttributes *m_pServerAttributes;

	CStr m_Password;
	CStrW m_ServerPlayerName;

	static CAttributeMap::UpdateCallback AttributeUpdate;

protected:
	friend class CNetServerSession;

	// Try to add the session as a newly connected player. If that is not
	// possible, make the session an observer.
	//
	// Synchronized, safe to call from any thread
	//
	// Returns:
	//	true. The session has been allocated a player slot
	//	false: All player slots busy, the session should be made an observer
	bool AddNewPlayer(CNetServerSession *pSession);

	// Queue a command coming in from the wire. The command has been validated
	// by the caller.
	//
	// Synchronized, safe from any thread
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
	// Synchronized, safe to call from any thread
	//
	// Returns:
	//	true if the session should be made an observer
	//	false otherwise
	virtual bool AllowObserver(CNetServerSession *pSession);

	void BroadcastUnsafe(CNetMessage *);

public:
	CNetServer(CNetServerAttributes *pServerAttribs, CGame *pGame, CGameAttributes *pGameAttribs);

	static void GetDefaultListenAddress(CSocketAddress &address);
	PS_RESULT Bind(const CSocketAddress &address);
	
	inline void SetPassword(CStr password)
	{	m_Password=password;	}

	inline CStrW GetServerPlayerName()
	{	return m_ServerPlayerName; }

	inline ENetServerState GetServerState()
	{	return m_ServerState; }

	int StartGame();

	// Synchronized, safe to call from any thread
	void Broadcast(CNetMessage *);
};

extern CNetServer *g_NetServer;
extern CNetServerAttributes g_NetServerAttributes;

#endif // _Network_NetServer_H
