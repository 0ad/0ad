#ifndef _Network_NetClient_H
#define _Network_NetClient_H

#include <CStr.h>
#include <Network/Session.h>

#include "TurnManager.h"
#include "Game.h"
#include "scripting/ScriptableObject.h"

class CPlayer;

class CNetClient: public CNetSession, protected CTurnManager, public CJSObject<CNetClient>
{
	CStrW m_Password;

	CPlayer *m_pLocalPlayer;
	CGame *m_pGame;
	CGameAttributes *m_pGameAttributes;

	// JS event scripts
	CScriptObject m_OnStartGame;
	CScriptObject m_OnChat;
	CScriptObject m_OnConnectComplete;
	CScriptObject m_OnDisconnect;

protected:
	virtual void NewTurn();
	virtual void QueueLocalCommand(CNetMessage *pMsg);

	void StartGame();
	
public:
	CNetClient(CGame *pGame, CGameAttributes *pGameAttribs);
	virtual ~CNetClient();

	inline void SetLoginInfo(CStrW nick, CStr password)
	{
		m_Name=nick;
		m_Password=password;
	}
	
	static MessageHandler ConnectHandler;

	static MessageHandler BaseHandler; // Common to all connected states
	static MessageHandler HandshakeHandler;
	static MessageHandler AuthenticateHandler;

	static MessageHandler ChatHandler; // Common to pre-game and later
	static MessageHandler PreGameHandler;
	static MessageHandler InGameHandler;
	
	bool JSI_BeginConnect(JSContext *cx, uintN argc, jsval *argv);
};

extern CNetClient *g_NetClient;

#endif //_Network_NetClient_H
