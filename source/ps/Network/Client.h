#ifndef _Network_NetClient_H
#define _Network_NetClient_H

#include <CStr.h>
#include <Network/Session.h>

#include "TurnManager.h"
#include "Game.h"
class CPlayer;

class CNetClient: public CNetSession, protected CTurnManager
{
	CStr m_Password;

	CPlayer *m_pLocalPlayer;
	CGame *m_pGame;
	CGameAttributes *m_pGameAttributes;

protected:
	virtual void NewTurn();
	virtual void QueueLocalCommand(CNetMessage *pMsg);

	void StartGame();
	
public:
	CNetClient(CGame *pGame, CGameAttributes *pGameAttribs);

	inline void SetLoginInfo(CStrW nick, CStr password)
	{
		m_Name=nick;
		m_Password=password;
	}
	
	static MessageHandler ConnectHandler;
	static MessageHandler HandshakeHandler;
	static MessageHandler AuthenticateHandler;
	static MessageHandler PreGameHandler;
	static MessageHandler ChatHandler;
};

extern CNetClient *g_NetClient;

#endif //_Network_NetClient_H
