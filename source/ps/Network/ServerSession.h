/*
	CNetServerSession - the server's representation of a connected client
	
	AUTHOR: Simon Brenner <simon.brenner@home.se>
	
	DESCRIPTION:
		
*/

#ifndef _Network_ServerSession_H
#define _Network_ServerSession_H

#include "Network/Session.h"

class CNetServer;
class CPlayer;

class CNetServerSession: public CNetSession
{
	CNetServer *m_pServer;
	CPlayer *m_pPlayer;
	bool m_IsObserver;
	
protected:
	friend class CNetServer;

	inline void SetPlayer(CPlayer *pPlayer)
	{	m_pPlayer=pPlayer; }

public:
	virtual ~CNetServerSession();

	inline CNetServerSession(CNetServer *pServer, CSocketInternal *pInt, MessageHandler *pMsgHandler=HandshakeHandler):
		CNetSession(pInt, pMsgHandler),
		m_pServer(pServer),
		m_pPlayer(NULL),
		m_IsObserver(false)
	{}

	inline bool IsObserver()
	{	return m_IsObserver; }

	// Called by server when starting the game, after sending NMT_StartGame to
	// all connected clients.
	void StartGame();

	static MessageHandler BaseHandler;
	static MessageHandler HandshakeHandler;
	static MessageHandler AuthenticateHandler;
	static MessageHandler PreGameHandler;
	static MessageHandler ObserverHandler;
	static MessageHandler ChatHandler;
	static MessageHandler InGameHandler;
};

#endif
