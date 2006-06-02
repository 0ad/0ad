#ifndef _Network_NetClient_H
#define _Network_NetClient_H

#include "ps/CStr.h"
#include "Session.h"

#include "simulation/TurnManager.h"
#include "simulation/ScriptObject.h"
#include "scripting/ScriptableObject.h"
#include "ps/scripting/JSMap.h"

#include <map>

class CPlayerSlot;
class CPlayer;
class CGame;
class CGameAttributes;

class CNetClient: public CNetSession, protected CTurnManager, public CJSObject<CNetClient>
{
	class CServerSession: public CJSObject<CServerSession>
	{
		static void ScriptingInit();
		
	public:
		CServerSession(int sessionID, const CStrW &name);
	
		int m_SessionID;
		CStrW m_Name;
	};
	typedef std::map<int, CServerSession *> SessionMap;
	
	SessionMap m_ServerSessions;
	CJSMap<SessionMap> m_JSI_ServerSessions;

	CStrW m_Password;
	int m_SessionID;

	CPlayerSlot *m_pLocalPlayerSlot;
	CGame *m_pGame;
	CGameAttributes *m_pGameAttributes;

	// JS event scripts
	CScriptObject m_OnStartGame;
	CScriptObject m_OnChat;
	CScriptObject m_OnConnectComplete;
	CScriptObject m_OnDisconnect;
	CScriptObject m_OnClientConnect;
	CScriptObject m_OnClientDisconnect;

	void OnClientConnect(int sessionID, const CStrW &name);
	void OnClientDisconnect(int sessionID);

	// JS Interface Functions
	bool JSI_BeginConnect(JSContext *cx, uintN argc, jsval *argv);
	static void ScriptingInit();

protected:
	virtual void NewTurn();
	virtual void QueueLocalCommand(CNetMessage *pMsg);

	void StartGame();
	
public:
	CNetClient(CGame *pGame, CGameAttributes *pGameAttribs);
	virtual ~CNetClient();

	static MessageHandler ConnectHandler;

	static MessageHandler BaseHandler; // Common to all connected states
	static MessageHandler HandshakeHandler;
	static MessageHandler AuthenticateHandler;

	static MessageHandler ChatHandler; // Common to pre-game and later
	static MessageHandler PreGameHandler;
	static MessageHandler InGameHandler;
};

extern CNetClient *g_NetClient;

#endif //_Network_NetClient_H
