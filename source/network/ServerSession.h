/*
	CNetServerSession - the server's representation of a connected client
	
	DESCRIPTION:
		
*/

#ifndef INCLUDED_NETWORK_SERVERSESSION
#define INCLUDED_NETWORK_SERVERSESSION

#include "Session.h"
#include "scripting/ScriptableObject.h"

class CNetServer;
class CPlayer;
class CPlayerSlot;

class CNetServerSession: public CNetSession, public CJSObject<CNetServerSession>
{
	CNetServer *m_pServer;
	CPlayer *m_pPlayer;
	CPlayerSlot *m_pPlayerSlot;
	bool m_IsObserver;
	int m_ID;
	bool m_ReadyForTurn;	// Is the last turn acknowledged?
	
	// JS INTERFACE
	bool JSI_Close(JSContext *cx, uintN argc, jsval *argv);
	
protected:
	friend class CNetServer;

	inline void SetPlayer(CPlayer *pPlayer)
	{	m_pPlayer=pPlayer; }
	
	inline void SetPlayerSlot(CPlayerSlot *pPlayerSlot)
	{	m_pPlayerSlot=pPlayerSlot; }
	
	inline void SetID(int id)
	{	m_ID=id; }

public:
	CNetServerSession(CNetServer *pServer, CSocketInternal *pInt, MessageHandler *pMsgHandler=HandshakeHandler);
	virtual ~CNetServerSession();

	static void ScriptingInit();

	inline bool IsObserver()
	{	return m_IsObserver; }
	inline CPlayer *GetPlayer()
	{	return m_pPlayer; }
	inline CPlayerSlot *GetPlayerSlot()
	{	return m_pPlayerSlot; }
	inline int GetID()
	{	return m_ID; }
	inline bool IsReadyForTurn()
	{	return m_ReadyForTurn; }
	inline void SetReadyForTurn(bool value)
	{	m_ReadyForTurn = value; }

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
